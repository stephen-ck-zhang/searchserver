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

#include "./CrawlFileTree.hpp"

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdlib>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <vector>

#include "./FileReader.hpp"

using std::string;
using std::vector;

namespace searchserver {

//////////////////////////////////////////////////////////////////////////////
// Internal helper functions and constants
//////////////////////////////////////////////////////////////////////////////

// Recursively descend into the passed-in directory, looking for files and
// subdirectories.  Any encountered files are processed via handle_file(); any
// subdirectories are recusively handled by handle_dir().
//
// Note that opendir()/readdir() iterates through a directory's entries in an
// unspecified order; since we need the ordering to be consistent in order
// to generate consistent DocTables and MemIndices, we do two passes over the
// contents: the first to extract the data necessary for populating
// entry_name_st and the second to actually handle the recursive call.
static void handle_dir(const string& dir_path,
                       DIR* dir_ptr,
                       WordIndex* word_index);

// Read and parse the specified file, then inject it into the MemIndex.
static void handle_file(const string& fpath, WordIndex* index);

//////////////////////////////////////////////////////////////////////////////
// Externally-exported functions
//////////////////////////////////////////////////////////////////////////////

bool crawl_filetree(const string& root_dir, WordIndex* index) {
  struct stat root_stat;
  DIR* rid = nullptr;

  // Verify we got some valid args.
  if (index == nullptr) {
    return false;
  }

  // Verify that root_dir is a directory.
  if (stat(root_dir.c_str(), &root_stat) == -1) {
    // We got some kind of error stat'ing the file. Give up
    // and return an error.
    return false;
  }

  if (!S_ISDIR(root_stat.st_mode)) {
    // It isn't a directory, so give up.
    return false;
  }

  // Try to open the directory using opendir().  If we fail, (e.g., we don't
  // have permissions on the directory), return a failure. ("man 3 opendir")
  rid = opendir(root_dir.c_str());
  if (rid == nullptr) {
    return false;
  }

  // Begin the recursive handling of the directory.
  handle_dir(root_dir, rid, index);

  // All done.  Release and/or transfer ownership of resources.
  closedir(rid);
  return true;
}

//////////////////////////////////////////////////////////////////////////////
// Internal helper functions
//////////////////////////////////////////////////////////////////////////////

static void handle_dir(const string& dir_path, DIR* dir_ptr, WordIndex* index) {
  // We make two passes through the directory.  The first gets the list of
  // all the metadata necessary to process its entries; the second iterates
  // does the actual recursive descent.

  struct dirent* dirent = nullptr;
  struct stat sit;

  // Use the "readdir()" system call to read the directory entries in a
  // loop ("man 3 readdir").  Exit out of the loop when we reach the end
  // of the directory.

  // First pass, to populate the "entries" list of item metadata.
  for (dirent = readdir(dir_ptr); dirent != nullptr;
       dirent = readdir(dir_ptr)) {
    // If the directory entry is named "." or "..", ignore it.  Use the C
    // "continue" expression to begin the next iteration of the loop.  What
    // field in the dirent could we use to find out the name of the entry?
    // How do you compare strings in C?
    if ((strcmp(static_cast<const char*>(dirent->d_name), ".") == 0) ||
        (strcmp(static_cast<const char*>(dirent->d_name), "..") == 0)) {
      // We need to pre-emptively decrement 'i' so that we don't erroneously
      // include "." and ".." in our final entry count.
      continue;
    }

    // We need to append the name of the file to the name of the directory
    // we're in to get the full filename. So, we'll malloc space for:
    //     dirpath + "/" + dirent->d_name + '\0'
    string path = dir_path;
    string entry_name = static_cast<const char*>(dirent->d_name);
    if (dir_path.back() != '/') {
      path += '/';
    }
    path += entry_name;

    // Use the "stat()" system call to ask the operating system to give us
    // information about the file identified by the directory entry (see
    // also "man 2 stat").
    if (stat(path.c_str(), &sit) == 0) {
      // Test to see if the file is a "regular file" using the S_ISREG() macro
      // described in the stat man page. If so, we'll process the file by
      // eventually invoking the handle_file() private helper function in our
      // second pass.
      //
      // On the other hand, if the file turns out to be a directory (which you
      // can find out using the S_ISDIR() macro described on the same page),
      // then we'll need to recursively process it using/ handle_dir() in our
      // second pass.
      //
      // If it is neither, skip the file.

      if (S_ISREG(sit.st_mode)) {
        handle_file(path, index);
      } else if (S_ISDIR(sit.st_mode)) {
        DIR* sub_dir = opendir(path.c_str());
        if (sub_dir != nullptr) {
          handle_dir(path, sub_dir, index);
          closedir(sub_dir);
        }
      } else {
        // This is neither a file nor a directory, so ignore it.  As before,
        // we pre-emptively decrement 'i' so that it doesn't get included
        // in our final entry count.
        continue;
      }
    }
  }
}

static void handle_file(const string& fpath, WordIndex* index) {
  // TODO: implement

  // Read the contents of the specified file into a string

  // Search the string for all tokens, where anything that is
  // not an alphabetic character is considered a delimiter
  // A delimiter marks the end of a token and is not considered a valid part of
  // the token

  // Record each non empty token as a word into the Wordindex specified by
  // *index

  // Your implementation should also be case in-sensitive and record every word
  // in all lower-case

  FileReader file = FileReader(fpath);
  string contents;
  file.read_file(&contents);

  vector<string> tokens;
  boost::split(tokens, contents, boost::is_any_of(" \t\n\r\f\v"),
               boost::token_compress_on);

  for (const string& token : tokens) {
    string word;
    for (char tok : token) {
      if (isalpha(tok) != 0) {
        word += static_cast<char>(tolower(tok));
      } else {
        if (!word.empty()) {
          index->record(word, fpath);
          word.clear();
        }
      }
    }
    if (!word.empty()) {
      index->record(word, fpath);
    }
  }
}

}  // namespace searchserver
