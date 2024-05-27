/*
 * Copyright ©2024 Travis McGaha.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Pennsylvania
 * CIT 5950 for use solely during Spring Semester 2024 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

// This file contains a number of HTTP and HTML parsing routines
// that come in useful throughput the assignment.

#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <iostream>
#include <vector>
#include "./HttpUtils.hpp"

using namespace std;

using boost::algorithm::replace_all;

namespace searchserver {

bool is_path_safe(const string& root_dir, const string& test_file) {
  // Get the absolute path of the root directory.
  char root_dir_abs[PATH_MAX];
  if (!realpath(root_dir.c_str(), root_dir_abs)) {
    return false;
  }

  // Get the absolute path of the test file.
  char test_file_abs[PATH_MAX];
  if (!realpath(test_file.c_str(), test_file_abs)) {
    return false;
  }

  // Check if the test file is within the root directory.
  if (strlen(root_dir_abs) >= strlen(test_file_abs)) {
    return false;
  }

  if (strncmp(root_dir_abs, test_file_abs, strlen(root_dir_abs)) != 0) {
    return false;
  }

  // Check for "." and ".." in the path for the test file.
  char* path_component = strtok(test_file_abs, "/");
  while (path_component != nullptr) {
    if (strcmp(path_component, ".") == 0 || strcmp(path_component, "..") == 0) {
      return false;
    }
    path_component = strtok(nullptr, "/");
  }

  // Check if the test file path is not an absolute path.
  if (test_file_abs[strlen(root_dir_abs)] != '\0' &&
      test_file_abs[strlen(root_dir_abs)] != '/') {
    return false;
  }

  return true;
}

string escape_html(const string& from) {
  string ret = from;
  // Read through the passed in string, and replace any unsafe
  // html tokens with the proper escape codes. The characters
  // that need to be escaped in HTML are the same five as those
  // that need to be escaped for XML documents. You can see an
  // example in the comment for this function in HttpUtils.h and
  // the rest of the characters that need to be replaced can be
  // looked up online.

  replace_all(ret, "&", "&amp;");
  replace_all(ret, "<", "&lt;");
  replace_all(ret, ">", "&gt;");
  replace_all(ret, "\"", "&quot;");
  replace_all(ret, "'", "&apos;");
  return ret;
}

// Look for a "%XY" token in the string, where XY is a
// hex number.  Replace the token with the appropriate ASCII
// character, but only if 32 <= dec(XY) <= 127.
string decode_URI(const string& from) {
  string retstr;

  // Loop through the characters in the string.
  for (unsigned int pos = 0; pos < from.length(); pos++) {
    // note: use pos+n<from.length() instead of pos<from.length-n
    // to avoid overflow problems with unsigned int values
    char c1 = from[pos];
    char c2 = (pos + 1 < from.length()) ? toupper(from[pos + 1]) : ' ';
    char c3 = (pos + 2 < from.length()) ? toupper(from[pos + 2]) : ' ';

    // Special case the '+' for old encoders.
    if (c1 == '+') {
      retstr.append(1, ' ');
      continue;
    }

    // Is this an escape sequence?
    if (c1 != '%') {
      retstr.append(1, c1);
      continue;
    }

    // Yes.  Are the next two characters hex digits?
    if (!((('0' <= c2) && (c2 <= '9')) || (('A' <= c2) && (c2 <= 'F')))) {
      retstr.append(1, c1);
      continue;
    }
    if (!((('0' <= c3) && (c3 <= '9')) || (('A' <= c3) && (c3 <= 'F')))) {
      retstr.append(1, c1);
      continue;
    }

    // Yes.  Convert to a code.
    uint8_t code = 0;
    if (c2 >= 'A') {
      code = 16 * (10 + (c2 - 'A'));
    } else {
      code = 16 * (c2 - '0');
    }
    if (c3 >= 'A') {
      code += 10 + (c3 - 'A');
    } else {
      code += (c3 - '0');
    }

    // Is the code reasonable?
    if (!((code >= 32) && (code <= 127))) {
      retstr.append(1, c1);
      continue;
    }

    // Great!  Convert and append.
    retstr.append(1, static_cast<char>(code));
    pos += 2;
  }
  return retstr;
}

void URLParser::parse(const string& url) {
  url_ = url;

  // Split the URL into the path and the args components.
  vector<string> ps;
  boost::split(ps, url, boost::is_any_of("?"));
  if (ps.size() < 1)
    return;

  // Store the URI-decoded path.
  path_ = decode_URI(ps[0]);

  if (ps.size() < 2)
    return;

  // Split the args into each field=val; chunk.
  vector<string> vals;
  boost::split(vals, ps[1], boost::is_any_of("&"));

  // Iterate through the chunks.
  for (unsigned int i = 0; i < vals.size(); i++) {
    // Split the chunk into field, value.
    string val = vals[i];
    vector<string> fv;
    boost::split(fv, val, boost::is_any_of("="));
    if (fv.size() == 2) {
      // Add the field, value to the args_ map.
      args_[decode_URI(fv[0])] = decode_URI(fv[1]);
    }
  }
}

int wrapped_read(int fd, std::string* out) {
  int res;
  char buffer[1024];
  while (1) {
    res = read(fd, buffer, 1024);
    if (res == -1) {
      if ((errno == EAGAIN) || (errno == EINTR))
        continue;
    }
    break;
  }
  *out += std::string(buffer, res);
  return res;
}

int wrapped_write(int fd, const string& buf) {
  int res;
  size_t written_so_far = 0;

  while (written_so_far < buf.size()) {
    res = write(fd, buf.c_str() + written_so_far, buf.size() - written_so_far);
    if (res == -1) {
      if ((errno == EAGAIN) || (errno == EINTR))
        continue;
      break;
    }
    if (res == 0)
      break;
    written_so_far += res;
  }
  return written_so_far;
}

bool connect_to_server(const string& host_name,
                       uint16_t port_num,
                       int* client_fd) {
  struct addrinfo hints, *results, *r;
  int client_sock, ret_val;
  char port_str[10];

  // Convert the port number to a C-style string.
  snprintf(port_str, sizeof(port_str), "%hu", port_num);

  // Zero out the hints data structure using memset.
  memset(&hints, 0, sizeof(hints));

  // Indicate we're happy with either AF_INET or AF_INET6 addresses.
  hints.ai_family = AF_UNSPEC;

  // Constrain the answers to SOCK_STREAM addresses.
  hints.ai_socktype = SOCK_STREAM;

  // Do the lookup.
  if ((ret_val = getaddrinfo(host_name.c_str(), port_str, &hints, &results)) !=
      0) {
    cerr << "getaddrinfo failed: ";
    cerr << gai_strerror(ret_val) << endl;
    return false;
  }

  // Loop through, trying each out until one succeeds.
  for (r = results; r != nullptr; r = r->ai_next) {
    // Try manufacturing the socket.
    if ((client_sock = socket(r->ai_family, SOCK_STREAM, 0)) == -1) {
      continue;
    }
    // Try connecting to the peer.
    if (connect(client_sock, r->ai_addr, r->ai_addrlen) == -1) {
      continue;
    }
    *client_fd = client_sock;
    freeaddrinfo(results);
    return true;
  }
  freeaddrinfo(results);
  return false;
}

uint16_t rand_port() {
  uint16_t portnum = 10000;
  portnum += ((uint16_t)getpid()) % 25000;
  portnum += ((uint16_t)rand()) % 5000;  // NOLINT(runtime/threadsafe_fn)
  return portnum;
}

}  // namespace searchserver
