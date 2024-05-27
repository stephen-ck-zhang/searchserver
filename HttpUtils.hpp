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

#ifndef HTTPUTILS_HPP_
#define HTTPUTILS_HPP_

#include <cstdint>

#include <string>
#include <utility>
#include <map>

namespace searchserver {

// This function tests whether the file "test_file" is contained below
// the root directory "root_dir".  If so, i.e., if the path is safe,
// the function returns true.  Otherwise, returns false.
//
// So, for example, the following combination is safe:
//
//   root_dir:  "test_files"
//   test_file: "test_files/ok/bar"
//
// but the following is not:
//
//   root_dir:  "test_files"
//   test_file: "libhw1/libhw1.a"
//
// nor is:
//
//   root_dir:  "test_files"
//   test_file: "test_files/../libhw1/libhw1.a"
//
bool is_path_safe(const std::string &root_dir, const std::string &test_file);

// This function performs HTML escaping in place.  It scans a string
// for dangerous HTML tokens (such as "<") and replaces them with the
// escaped HTML equivalent (such as "&lt;").  This helps to prevent
// XSS attacks.
std::string escape_html(const std::string &from);

// This function performs URI decoding.  It scans a string for
// the "%" escape character and converts the token to the
// appropriate ASCII character.  See the wikipedia article on
// URL encoding for an explanation of what's going on here:
//
//    http://en.wikipedia.org/wiki/Percent-encoding
//
std::string decode_URI(const std::string &from);

// A URL that's part of a web request has the following structure:
//
//   /foo/bar/baz?field=value&field2=value2
//
//      path     ?   args
//
// This class accepts a URL and splits it into these components and
// URIDecode()'s them, allowing the caller to access the components
// through convenient methods.
class URLParser {
 public:
  URLParser() { }
  virtual ~URLParser() { }

  void parse(const std::string &url);

  // Return the "path" component of the url, post-uri-decoding.
  std::string path() const { return path_; }

  // Return the "args" component of the url post-uri-decoding.
  // The args component is parsed into a map from field to value.
  std::map<std::string, std::string> args() const { return args_; }

 private:
  std::string url_;
  std::string path_;
  std::map<std::string, std::string> args_;
};

// A wrapper around the write() system call that shields the caller
// from dealing with the ugly issues of partial writes, EINTR, EAGAIN,
// and so on.
//
// Writes the specified string to the file descriptor fd, not including
// the null terminator.  Blocks the caller until either all bytes have been written,
// or an error is encountered.  Returns the total number of bytes written;
// if this number is less than write_len, it's because some fatal error
// was encountered, like the connection being dropped.
int wrapped_write(int fd, const std::string& buf);

// A wrapper around the read() system call that shields the caller
// from dealing with the ugly issues of partial reads, EINTR, EAGAIN,
// and so on.
//
// Reads as many bytes from the file descriptor fd onto the end of
// the buffer string "buf".  Returns the number of bytes actually
// read.  On fatal error, returns -1.  If EOF is hit and no
// bytes have been read, returns 0.  Might read fewer bytes
// than requested.
int wrapped_read(int fd, std::string *out);

// Below is used to test server socket

// A convenience routine to manufacture a (blocking) socket to the
// host_name and port number provided as arguments.  Hostname can
// be a DNS name or an IP address, in string form.  On success,
// returns a file descriptor thorugh "client_fd" and returns true.
// On failure, returns false.  Caller is responsible for close()'ing
// the file descriptor.
bool connect_to_server(const std::string &host_name, uint16_t port_num,
                    int *client_fd);

// Return a randomly generated port number between 10000 and 40000.
uint16_t rand_port();

}  // namespace searchserver

#endif  // HTTPUTILS_HPP_
