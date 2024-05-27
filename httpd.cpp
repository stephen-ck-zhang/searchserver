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

#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <list>
#include <string>

#include "./ServerSocket.hpp"
#include "./HttpServer.hpp"
#include "./CrawlFileTree.hpp"

using std::cerr;
using std::cout;
using std::endl;
using std::list;
using std::string;

// Print out program usage, and exit() with EXIT_FAILURE.
static void Usage(char *prog_name);

// Parses the command-line arguments, invokes Usage() on failure.
// "port" is a return parameter to the port number to listen on,
// "path" is a return parameter to the directory containing
// our static files, and "indices" is a return parameter to a
// list of index filenames.  Ensures that the path is a readable
// directory, and the index filenames are readable, and if not,
// invokes Usage() to exit.
static void GetPortAndPath(int argc,
                    char **argv,
                    uint16_t *port,
                    string *path);

int main(int argc, char **argv) {
  // Print out welcome message.
  cout << "initializing:" << endl;
  cout << "  parsing port number and static files directory..." << endl;

  // Ignore the SIGPIPE signal, otherwise we'll crash out if a client
  // disconnects unexpectedly.
  signal(SIGPIPE, SIG_IGN);

  // Get the port number and list of index files.
  uint16_t port_num;
  string static_dir;
  GetPortAndPath(argc, argv, &port_num, &static_dir);
  cout << "    port: " << port_num << endl;
  cout << "    path: " << static_dir << endl;

  searchserver::WordIndex *index = new searchserver::WordIndex();
 
  if (!searchserver::crawl_filetree(static_dir, index)) {
    cerr << " failed to crawl the file directory" << endl;
    return EXIT_FAILURE;
  }

  // Run the server.
  searchserver::HttpServer hs(port_num, static_dir, index);
  if (!hs.run()) {
    cerr << "  server failed to run!?" << endl;
  }

  delete index;

  cout << "server completed!  Exiting." << endl;
  return EXIT_SUCCESS;
}


static void Usage(char *prog_name) {
  cerr << "Usage: " << prog_name << " port staticfiles_directory";
  cerr << endl;
  exit(EXIT_FAILURE);
}

static void GetPortAndPath(int argc,
                    char **argv,
                    uint16_t *port,
                    string *path) {
  // Be sure to check a few things:
  //  (a) that you have a sane number of command line arguments
  //  (b) that the port number is reasonable
  //  (c) that "path" (i.e., argv[2]) is a readable directory

  // STEP 1:
  // Do we have the right number of command line arguments?
  if (argc != 3) {
    cerr << endl;
    Usage(argv[0]);
  }

  // Try to get the port number.
  if (sscanf(argv[1], "%hu", port) != 1) {
    cerr << endl << argv[1] << " isn't a valid port number." << endl;
    Usage(argv[0]);
  }

  // Test to see if "path" is a readable directory.
  struct stat fs;
  if ((stat(argv[2], &fs) == -1) ||
      (!S_ISDIR(fs.st_mode))) {
    cerr << endl << argv[2] << " isn't a directory." << endl;
    Usage(argv[0]);
  }

  DIR *d = opendir(argv[2]);
  if (d == nullptr) {
    cerr << endl << argv[2] << " isn't a readable directory." << endl;
    Usage(argv[0]);
  }

  closedir(d);
  *path = argv[2];
}

