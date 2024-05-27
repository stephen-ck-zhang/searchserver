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

#ifndef HTTPSERVER_HPP_
#define HTTPSERVER_HPP_

#include <cstdint>
#include <string>
#include <list>

#include "./ThreadPool.hpp"
#include "./ServerSocket.hpp"
#include "./WordIndex.hpp"

namespace searchserver {

// The HttpServer class contains the main logic for the web server.
class HttpServer {
 public:
  // Creates a new HttpServer object for port "port" and serving
  // files out of path "staticfile_dirpath".  The index for
  // query processing is loaded already and onwership of
  // the index is not taken.
  explicit HttpServer(uint16_t port,
                      const std::string &static_file_dir_path,
                      WordIndex* index)
    : socket_(port), static_file_dir_path_(static_file_dir_path),
      index_(index) { }

  // The destructor closes the listening socket if it is open and
  // also kills off any threads in the threadpool.
  virtual ~HttpServer() { }

  // Creates a listening socket for the server and launches it, accepting
  // connections and dispatching them to worker threads.  Returns
  // "true" if the server was able to start and run, "false" otherwise.
  // The server continues to run until a kill command is used to send
  // a SIGTERM signal to the server process (i.e., kill pid), so this function
  // doesn't really return.
  bool run();

 private:
  ServerSocket socket_;
  std::string static_file_dir_path_;
  WordIndex* index_;
  static const int kNumThreads;
};

// A task for the ThreadPool
// When the server accpets a new connection, it must
// initialize one of these tasks to that the thread has
// the information it needs to handle the client
class HttpServerTask : public ThreadPool::Task {
 public:
  explicit HttpServerTask(ThreadPool::thread_task_fn f)
    : ThreadPool::Task(f) { }

  int client_fd;
  uint16_t c_port;
  std::string c_addr, c_dns, s_addr, s_dns;
  std::string base_dir;
  WordIndex *index;
};

}  // namespace searchserver

#endif  // HTTPSERVER_HPP_
