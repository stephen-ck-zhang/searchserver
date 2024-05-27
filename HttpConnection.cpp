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

#include <boost/algorithm/string.hpp>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "./HttpConnection.hpp"
#include "./HttpRequest.hpp"
#include "./HttpResponse.hpp"
#include "./HttpUtils.hpp"

using namespace std;

namespace searchserver {

static const char* headerEnd = "\r\n\r\n";
static const int headerEndLen = 4;

// Read and parse the next request from the file descriptor fd_,
// storing the state in the output parameter "request."  Returns
// true if a request could be read, false if the parsing failed
// for some reason, in which case the caller should close the
// connection.
bool HttpConnection::next_request(HttpRequest* request) {
  // Find the position of the end of the header in the buffer
  size_t header_end_pos = buffer_.find(headerEnd);

  // If the end of the header is not found, read more data until it is found
  while (header_end_pos == string::npos) {
    // Read data into the buffer using wrapped_read
    int bytes_read = wrapped_read(fd_, &buffer_);
    if (bytes_read == 0) {
      return false;  // Connection dropped
    } else if (bytes_read == -1) {
      return false;  // Error reading
    } else {
      // Update header_end_pos after reading more data
      header_end_pos = buffer_.find(headerEnd);
    }
  }

  // Parse the request from the buffer
  if (!parse_request(buffer_.substr(0, header_end_pos), request)) {
    return false;  // Error parsing request
  }

  // Update the buffer to remove the parsed request
  buffer_ = buffer_.substr(header_end_pos + headerEndLen);

  return true;
}

bool HttpConnection::write_response(const HttpResponse& response) {
  string response_str = response.GenerateResponseString();
  int bytes_written = wrapped_write(fd_, response_str);
  if (bytes_written != static_cast<int>(response_str.length())) {
    return false;
  }
  return true;
}

bool HttpConnection::parse_request(const string& request, HttpRequest* out) {
  HttpRequest req("/");

  vector<string> lines;
  boost::split(lines, request, boost::is_any_of("\r\n"),
               boost::token_compress_on);

  if (lines.empty()) {
    req.set_uri("/");
    return false;
  }

  for (size_t i = 1; i < lines.size(); i++) {
    boost::trim(lines[i]);
  }

  vector<string> parts;
  boost::split(parts, lines[0], boost::is_any_of(" "),
               boost::token_compress_on);

  if (parts.size() != 3 || parts[0] != "GET") {
    return false;
  }

  req.set_uri(parts[1]);

  for (size_t i = 1; i < lines.size(); i++) {
    size_t pos = lines[i].find(":");

    if (pos == string::npos) {
      return false;
    }

    string header_name = lines[i].substr(0, pos);
    boost::to_lower(header_name);
    boost::trim(header_name);
    string header_value = lines[i].substr(pos + 1);
    boost::trim(header_value);
    req.AddHeader(header_name, header_value);
  }

  *out = req;
  return true;
}

}  // namespace searchserver
