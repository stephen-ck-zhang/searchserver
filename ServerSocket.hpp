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

#ifndef SERVERSOCKET_HPP_
#define SERVERSOCKET_HPP_

#include <netdb.h>       // for AF_UNSPEC, AF_INET, AF_INET6
#include <cstdint>      // for uint16_t, etc.
#include <sys/types.h>   // for AF_UNSPEC, AF_INET, AF_INET6
#include <sys/socket.h>  // for AF_UNSPEC, AF_INET, AF_INET6
#include <string>

namespace searchserver {

// A ServerSocket class abstracts away the messy details of creating a
// TCP listening socket at a specific port and on a (hopefully)
// externally visible IP address.  As well, a ServerSocket helps
// customers accept incoming client connections on the listening
// socket.
//
// ServerSocket only supports IPv6, you do not have to support IPv4
class ServerSocket {
 public:
  // This constructor creates a new ServerSocket object and associates
  // it with the provided port number.  The constructor doesn't create
  // a socket yet; it just memorizes the given port.
  explicit ServerSocket(uint16_t port);

  // The destructor closes the listening socket if it is open.
  virtual ~ServerSocket();

  // This function causes the ServerSocket to attempt to create a
  // listening socket and to bind it to the given port number on
  // whatever IP address the host OS recommends for us.  The caller
  // provides:
  //
  // On failure this function returns false.  On success, it returns
  // true, sets listen_sock_fd_ to be the file descriptor for the
  // listening socket, and also returns (via an output parameter):
  //
  // - listen_fd: the file descriptor for the listening socket.
  //              which should be the same value as listen_fd_
  bool bind_and_listen(int *listen_fd);

  // This function causes the ServerSocket to attempt to accept
  // an incoming connection from a client.  On failure, returns false.
  // On success, it returns true, and also returns (via output
  // parameters) the following:
  //
  // - accepted_fd: the file descriptor for the new client connection.
  //   The customer is responsible for close()'ing this socket when it
  //   is done with it.
  //
  // - client_addr: a C++ string object containing a printable
  //   representation of the IP address the client connected from.
  //
  // - client_port: a uint16_t containing the port number the client
  //   connected from.
  //
  // - client_dnsname: a C++ string object containing the DNS name
  //   of the client.
  //
  // - server_addr: a C++ string object containing a printable
  //   representation of the server IP address for the connection.
  //
  // - server_dnsname: a C++ string object containing the DNS name
  //   of the server.
  bool accept_client(int *accepted_fd,
                     std::string *client_addr, uint16_t *client_port,
                     std::string *client_dns_name, std::string *server_addr,
                     std::string *server_dns_name) const;

 private:
  uint16_t port_;
  int listen_sock_fd_;
};

}  // namespace searchserver

#endif  // SERVERSOCKET_HPP_