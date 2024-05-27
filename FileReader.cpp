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

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

#include "./FileReader.hpp"
#include "./HttpUtils.hpp"

using std::string;

namespace searchserver {

bool FileReader::read_file(string* str) {
  // Read the file into memory, and store the file contents in the
  // output parameter "str."  Be careful to handle binary data
  // correctly; i.e., you probably want to use the two-argument
  // constructor to std::string (the one that includes a length as a
  // second argument).

  // TODO: implement
  // Check if the file exists
  if (access(fname_.c_str(), F_OK) == -1) {
    return false;
  }

  // Open the file
  int fd = open(fname_.c_str(), O_RDONLY);
  if (fd == -1) {
    return false;
  }

  // Get the file size
  struct stat st;
  if (fstat(fd, &st) == -1) {
    close(fd);
    return false;
  }

  // Read the file into memory
  char* buf = new char[st.st_size];
  if (buf == nullptr) {
    close(fd);
    return false;
  }

  ssize_t bytes_read = read(fd, buf, st.st_size);
  if (bytes_read == -1) {
    delete[] buf;
    close(fd);
    return false;
  }

  // Store the file contents in "str"
  *str = string(buf, st.st_size);
  delete[] buf;
  close(fd);
  return true;
}

}  // namespace searchserver
