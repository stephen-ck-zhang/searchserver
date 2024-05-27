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

#include <arpa/inet.h>   // for inet_ntop()
#include <errno.h>       // for errno, used by strerror()
#include <netdb.h>       // for getaddrinfo()
#include <sys/socket.h>  // for socket(), getaddrinfo(), etc.
#include <sys/types.h>   // for socket(), getaddrinfo(), etc.
#include <unistd.h>      // for close(), fcntl()
#include <cstdio>        // for snprintf()
#include <cstring>       // for memset, strerror()
#include <iostream>      // for std::cerr, etc.

#include "./ServerSocket.hpp"

using namespace std;

namespace searchserver {

void PrintOut(int fd, struct sockaddr* addr, size_t addrlen);

ServerSocket::ServerSocket(uint16_t port) {
  port_ = port;
  listen_sock_fd_ = -1;
}

ServerSocket::~ServerSocket() {
  // Close the listening socket if it's not zero.  The rest of this
  // class will make sure to zero out the socket if it is closed
  // elsewhere.
  if (listen_sock_fd_ != -1)
    close(listen_sock_fd_);
  listen_sock_fd_ = -1;
}

bool ServerSocket::bind_and_listen(int* listen_fd) {
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));

  hints.ai_family = AF_INET6;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE | AI_V4MAPPED;
  hints.ai_protocol = IPPROTO_TCP;

  string portStr = to_string(port_);
  struct addrinfo* result;

  int res = getaddrinfo(nullptr, portStr.c_str(), &hints, &result);

  if (res != 0) {
    cerr << "getaddrinfo() failed: " << gai_strerror(res) << std::endl;
    return false;
  }

  int sock_fd = -1;
  for (struct addrinfo* rp = result; rp != nullptr; rp = rp->ai_next) {
    sock_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

    if (sock_fd == -1) {
      cerr << "socket() failed: " << strerror(errno) << endl;
      continue;
    }

    int optval = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    if (bind(sock_fd, rp->ai_addr, rp->ai_addrlen) == 0) {
      PrintOut(sock_fd, rp->ai_addr, rp->ai_addrlen);
      break;
    }

    close(sock_fd);
    sock_fd = -1;
  }

  freeaddrinfo(result);

  if (sock_fd == -1) {
    cerr << "Failed to bind to any address: " << strerror(errno) << endl;
    return false;
  }

  if (listen(sock_fd, SOMAXCONN) != 0) {
    cerr << "Failed to mark socket as listening: " << strerror(errno) << endl;
    close(sock_fd);
    return false;
  }

  listen_sock_fd_ = sock_fd;
  *listen_fd = sock_fd;

  return true;
}

bool ServerSocket::accept_client(int* accepted_fd,
                                 std::string* client_addr,
                                 uint16_t* client_port,
                                 std::string* client_dns_name,
                                 std::string* server_addr,
                                 std::string* server_dns_name) const {
  // Accept a new connection on the listening socket listen_sock_fd_.
  // (Block until a new connection arrives.)  Return the newly accepted
  // socket, as well as information about both ends of the new connection,
  // through the various output parameters.

  // Loop forever, accepting a connection from a client and doing
  // an echo trick to it.
  struct sockaddr_storage caddr;
  socklen_t caddr_len = sizeof(caddr);
  int client_fd = -1;
  while (1) {
    client_fd = accept(listen_sock_fd_,
                       reinterpret_cast<struct sockaddr*>(&caddr), &caddr_len);

    if (client_fd < 0) {
      if ((errno == EINTR) || (errno == EAGAIN)) {
        continue;
      }
      std::cerr << "Failure on accept: " << strerror(errno) << std::endl;
      return false;
    }
    break;
  }

  if (client_fd < 0) {
    std::cerr << "Failure on accept: " << strerror(errno) << std::endl;
    return false;
  }

  *accepted_fd = client_fd;

  // Get the client's IP address and port number.
  char client_ip[INET6_ADDRSTRLEN];
  struct sockaddr_in6* caddr6 = reinterpret_cast<struct sockaddr_in6*>(&caddr);
  inet_ntop(AF_INET6, &(caddr6->sin6_addr), client_ip, INET6_ADDRSTRLEN);
  *client_addr = client_ip;
  *client_port = htons(caddr6->sin6_port);

  // Get the client's DNS name.
  char client_dns[NI_MAXHOST];
  int res = getnameinfo(reinterpret_cast<struct sockaddr*>(&caddr), caddr_len,
                        client_dns, NI_MAXHOST, nullptr, 0, 0);

  if (res != 0) {
    std::cerr << "getnameinfo() failed: " << gai_strerror(res) << std::endl;
    return false;
  }
  *client_dns_name = string(client_dns);

  // Get the server's IP address and port number.
  char server_ip[1024];
  server_ip[0] = '\0';

  struct sockaddr_in6 saddr;
  socklen_t saddr_len = sizeof(saddr);
  char addrb[INET6_ADDRSTRLEN];
  getsockname(client_fd, reinterpret_cast<struct sockaddr*>(&saddr),
              &saddr_len);
  inet_ntop(AF_INET6, &(saddr.sin6_addr), addrb, INET6_ADDRSTRLEN);
  getnameinfo(reinterpret_cast<struct sockaddr*>(&saddr), saddr_len, server_ip,
              sizeof(server_ip), nullptr, 0, 0);
  *server_addr = string(addrb);
  *server_dns_name = string(server_ip);
  return true;
}

void PrintOut(int fd, struct sockaddr* addr, size_t addrlen) {
  if (addr == nullptr) {
    std::cerr << "Invalid address passed to PrintOut" << std::endl;
    return;
  }

  std::cout << "Socket [" << fd << "] is bound to:" << std::endl;
  if (addr->sa_family == AF_INET6) {
    // Print out the IPV6 address and port

    char astring[INET6_ADDRSTRLEN];
    struct sockaddr_in6* in6 = reinterpret_cast<struct sockaddr_in6*>(addr);
    inet_ntop(AF_INET6, &(in6->sin6_addr), astring, INET6_ADDRSTRLEN);
    std::cout << " IPv6 address " << astring;
    std::cout << " and port " << ntohs(in6->sin6_port) << std::endl;

  } else {
    std::cout << "Unknown address family" << std::endl;
  }
}

}  // namespace searchserver
