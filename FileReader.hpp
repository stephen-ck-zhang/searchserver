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

#ifndef FILEREADER_HPP_
#define FILEREADER_HPP_

#include <string>

namespace searchserver {

// This class is used to read a file into memory and return its
// contents as a string.
class FileReader {
 public:
  // Constructs a file reader for the specified file
  FileReader(const std::string &fname)
    : fname_(fname) { }
  virtual ~FileReader() { }

  // Attempts to reads in the file specified by the constructor
  // arguments. If the file could not be found or could not be opened
  // returns false.  Otherwise, returns true and also returns 
  // the file contents through "str".
  bool read_file(std::string *str);

 private:
  std::string fname_;
};

}  // namespace searchserver

#endif  // FILEREADER_HPP_
