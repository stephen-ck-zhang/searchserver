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

#ifndef HTTPCONNECTION_HPP_
#define HTTPCONNECTION_HPP_

#include <cstdint>
#include <unistd.h>
#include <map>
#include <string>

#include "./HttpRequest.hpp"
#include "./HttpResponse.hpp"

namespace searchserver {

// The HttpConnection class represents a connection to a single client
class HttpConnection {
 public:
 
  // Constructs a new HttpConnection to handle the
  // connection to a client on the represented file descriptor
  explicit HttpConnection(int fd) : fd_(fd) { }
  
  // closes the connection to the client if it is still open
  virtual ~HttpConnection() {
    if (fd_ != -1) {
      close(fd_);
    }
    fd_ = -1;
  }

  // Read and parse the next request from the file descriptor fd_,
  // storing the state in the output parameter "request."  Returns
  // true if a request could be read, false if the parsing failed
  // for some reason, in which case the caller should close the
  // connection.
  bool next_request(HttpRequest *request);

  // Write the response to the file descriptor fd_.  Returns true
  // if the response was successfully written, false if the
  // connection experiences an error and should be closed.
  bool write_response(const HttpResponse &response);

 private:
  // A helper function to parse the contents of data read from
  // the HTTP connection. Returns true if the request was parsed
  // successfully and that request is returned through *request
  // and returns false if the Request is an invalid format
  bool parse_request(const std::string &request, HttpRequest *out);

  // The file descriptor associated with the client.
  int fd_;

  // A buffer storing data read from the client.
  // Used for the case where we read more data than we need to process a request
  // store the excess data read into the buffer so that next time we read, we can parse from here
  std::string buffer_;
};

}  // namespace searchserver

#endif  // HTTPCONNECTION_HPP_
