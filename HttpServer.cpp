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
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "./CrawlFileTree.hpp"
#include "./FileReader.hpp"
#include "./HttpConnection.hpp"
#include "./HttpRequest.hpp"
#include "./HttpServer.hpp"
#include "./HttpUtils.hpp"
#include "./WordIndex.hpp"

using std::cerr;
using std::cout;
using std::endl;
using std::list;
using std::map;
using std::string;
using std::stringstream;
using std::unique_ptr;
using std::vector;

namespace searchserver {
///////////////////////////////////////////////////////////////////////////////
// Constants, internal helper functions
///////////////////////////////////////////////////////////////////////////////
static const char* kFivegleStr =
    "<html><head><title>5950gle</title></head>\n"
    "<body>\n"
    "<center style=\"font-size:500%;\">\n"
    "<span style=\"position:relative;bottom:-0.33em;color:orange;\">5</span>"
    "<span style=\"color:red;\">9</span>"
    "<span style=\"color:gold;\">5</span>"
    "<span style=\"color:blue;\">g</span>"
    "<span style=\"color:green;\">l</span>"
    "<span style=\"color:red;\">e</span>\n"
    "</center>\n"
    "<p>\n"
    "<div style=\"height:20px;\"></div>\n"
    "<center>\n"
    "<form action=\"/query\" method=\"get\">\n"
    "<input type=\"text\" size=30 name=\"terms\" />\n"
    "<input type=\"submit\" value=\"Search\" />\n"
    "</form>\n"
    "</center><p>\n";

// static
const int HttpServer::kNumThreads = 100;

// This is the function that threads are dispatched into
// in order to process new client connections.
static void HttpServer_ThrFn(ThreadPool::Task* t);

// Given a request, produce a response.
static HttpResponse ProcessRequest(const HttpRequest& req,
                                   const string& base_dir,
                                   WordIndex* indices);

// Process a file request.
static HttpResponse ProcessFileRequest(const string& uri,
                                       const string& base_dir);

// Process a query request.
static HttpResponse ProcessQueryRequest(const string& uri, WordIndex* index);

///////////////////////////////////////////////////////////////////////////////
// HttpServer
///////////////////////////////////////////////////////////////////////////////
bool HttpServer::run(void) {
  // Create the server listening socket.
  int listen_fd;
  cout << "  creating and binding the listening socket..." << endl;
  if (!socket_.bind_and_listen(&listen_fd)) {
    cerr << endl << "Couldn't bind to the listening socket." << endl;
    return false;
  }

  // Spin, accepting connections and dispatching them.  Use a
  // threadpool to dispatch connections into their own thread.
  cout << "  accepting connections..." << endl << endl;
  ThreadPool tp(kNumThreads);
  while (1) {
    HttpServerTask* hst = new HttpServerTask(HttpServer_ThrFn);
    hst->base_dir = static_file_dir_path_;
    hst->index = index_;
    if (!socket_.accept_client(&hst->client_fd, &hst->c_addr, &hst->c_port,
                               &hst->c_dns, &hst->s_addr, &hst->s_dns)) {
      // The accept failed for some reason, so quit out of the server.
      // (Will happen when kill command is used to shut down the server.)
      break;
    }
    // The accept succeeded; dispatch it.
    tp.dispatch(hst);
  }
  return true;
}

static void HttpServer_ThrFn(ThreadPool::Task* t) {
  // Cast back our HttpServerTask structure with all of our new
  // client's information in it.
  unique_ptr<HttpServerTask> hst(static_cast<HttpServerTask*>(t));
  cout << "  client " << hst->c_dns << ":" << hst->c_port << " "
       << "(IP address " << hst->c_addr << ")"
       << " connected." << endl;

  // Read in the next request, process it, write the response.

  // Use the HttpConnection class to read and process the next
  // request from our current client, then write out our response.  If
  // the client sends a "Connection: close\r\n" header, then shut down
  // the connection -- we're done.
  //
  // Hint: the client can make multiple requests on our single connection,
  // so we should keep the connection open between requests rather than
  // creating/destroying the same connection repeatedly.

  // TODO: Implement
  // Create an HttpConnection object for the client's file descriptor
  HttpConnection connection(hst->client_fd);

  // Initialize a loop to keep the connection open
  while (true) {
    // Read the next request from the client
    HttpRequest request;
    if (!connection.next_request(&request)) {
      // Reading the request failed, break out of the loop and close the
      // connection
      break;
    }

    // Process the request and generate a response
    HttpResponse response = ProcessRequest(request, hst->base_dir, hst->index);

    // Write the response back to the client
    if (!connection.write_response(response)) {
      // Writing the response failed, break out of the loop and close the
      // connection
      break;
    }

    // Check if the request contains a "Connection: close" header
    if (request.GetHeaderValue("Connection") == "close") {
      // Client requested to close the connection, break out of the loop and
      // close the connection
      break;
    }
  }

  // Close the connection
  close(hst->client_fd);
}

static HttpResponse ProcessRequest(const HttpRequest& req,
                                   const string& base_dir,
                                   WordIndex* index) {
  // Is the user asking for a static file?
  if (req.uri().substr(0, 8) == "/static/") {
    return ProcessFileRequest(req.uri(), base_dir);
  }

  // The user must be asking for a query.
  return ProcessQueryRequest(req.uri(), index);
}

static HttpResponse ProcessFileRequest(const string& uri,
                                       const string& base_dir) {
  // The response we'll build up.
  HttpResponse ret;

  // Steps to follow:
  //  - use the URLParser class to figure out what filename
  //    the user is asking for. Note that we identify a request
  //    as a file request if the URI starts with '/static/'
  //
  //  - use the FileReader class to read the file into memory
  //
  //  - copy the file content into the ret.body
  //
  //  - depending on the file name suffix, set the response
  //    Content-type header as appropriate, e.g.,:
  //      --> for ".html" or ".htm", set to "text/html"
  //      --> for ".jpeg" or ".jpg", set to "image/jpeg"
  //      --> for ".png", set to "image/png"
  //      etc.
  //    You should support the file types mentioned above,
  //    as well as ".txt", ".js", ".css", ".xml", ".gif",
  //    and any other extensions to get bikeapalooza
  //    to match the solution server.
  //
  // be sure to set the response code, protocol, and message
  // in the HttpResponse as well.

  // TODO: Implement

  // Extract the filename from the URI
  string filename;
  if (uri.substr(0, 8) == "/static/") {
    filename =
        uri.substr(8);  // Remove '/static/' from the beginning of the URI
  } else {
    // Invalid file request, return a 404 response
    ret.set_protocol("HTTP/1.1");
    ret.set_response_code(404);
    ret.set_message("Not Found");
    ret.AppendToBody("<html><body>Invalid file request</body></html>\n");
    return ret;
  }

  // Use the FileReader class to read the file into memory
  string full_path = filename;
  FileReader file_reader(full_path);
  string file_content;
  if (!file_reader.read_file(&file_content)) {
    // If the file couldn't be read, return a 404 response
    ret.set_protocol("HTTP/1.1");
    ret.set_response_code(404);
    ret.set_message("Not Found");
    ret.AppendToBody("<html><body>Couldn't find file \"" +
                     escape_html(filename) + "\"</body></html>\n");
    return ret;
  }

  // Determine the content type based on the file extension
  string content_type;
  if (boost::ends_with(filename, ".html") ||
      boost::ends_with(filename, ".htm")) {
    content_type = "text/html";
  } else if (boost::ends_with(filename, ".jpg") ||
             boost::ends_with(filename, ".jpeg")) {
    content_type = "image/jpeg";
  } else if (boost::ends_with(filename, ".png")) {
    content_type = "image/png";
  } else if (boost::ends_with(filename, ".txt")) {
    content_type = "text/plain";
  } else if (boost::ends_with(filename, ".js")) {
    content_type = "application/javascript";
  } else if (boost::ends_with(filename, ".css")) {
    content_type = "text/css";
  } else if (boost::ends_with(filename, ".xml")) {
    content_type = "application/xml";
  } else if (boost::ends_with(filename, ".gif")) {
    content_type = "image/gif";
  } else {
    content_type = "application/octet-stream";
  }

  // Set the response code, protocol, message, body, and content type
  ret.set_protocol("HTTP/1.1");
  ret.set_response_code(200);
  ret.set_message("OK");
  ret.set_content_type(content_type);
  ret.AppendToBody(file_content);

  return ret;
}

static HttpResponse ProcessQueryRequest(const string& uri, WordIndex* index) {
  // The response we're building up.
  HttpResponse ret;

  // Add the 5950gle logo and the search box/button to the response body
  ret.AppendToBody(kFivegleStr);

  string search_query;
  if (!uri.empty() && uri.find("/query?terms=") == 0) {
    size_t pos = uri.find("=");
    if (pos != string::npos) {
      search_query = uri.substr(pos + 1);
    }
  }

  // If a search query is present, process it
  if (!search_query.empty()) {
    // Convert search query to lowercase
    boost::algorithm::to_lower(search_query);

    // Tokenize the search query
    vector<string> search_terms;
    boost::algorithm::split(search_terms, search_query, boost::is_any_of("+"));
    for (auto& term : search_terms) {
      term = boost::algorithm::trim_copy(term);
    }

    // Perform the search
    vector<Result> results = index->lookup_query(search_terms);

    // Add the search results to the response body
    ret.AppendToBody("<h2>Search results:</h2>\n");
    ret.AppendToBody("<p>" + std::to_string(results.size()) +
                     " results found for \"" + search_query + "\"</p>\n");
    for (const auto& result : results) {
      ret.AppendToBody("<p><a href=\"/static/" + result.doc_name + "\">" +
                       result.doc_name + "</a> (" +
                       std::to_string(result.rank) + ")</p>\n");
    }
  }

  // Set the content type and return the response
  ret.set_protocol("HTTP/1.1");
  ret.set_response_code(200);  // Assuming successful response
  ret.set_message("OK");
  ret.set_content_type("text/html");  // Set appropriate content type

  return ret;
}

}  // namespace searchserver