// parser.c - Implementation of HTTP request parser
#include "parser.h"
#include <string.h>
#include <stdio.h>

int parseHTTPRequest(const char *rawRequest, HTTPRequest *request) {
  // Example of rawRequest:
  // "GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n"

  if (rawRequest == NULL || request == NULL) return -1;

  // Temporary variables
  char method[MAX_METHOD_LEN];
  char path[MAX_PATH_LEN];

  // Use sscanf to extract method and path from the request line
  int parsed = sscanf(rawRequest, "%7s %255s", method, path);

  if (parsed != 2) {
    return -1; // Parsing failed
  }

  // Copy to request structure
  strncpy(request->method, method, MAX_METHOD_LEN);
  request->method[MAX_METHOD_LEN - 1] = '\0';

  strncpy(request->path, path, MAX_PATH_LEN);
  request->path[MAX_PATH_LEN - 1] = '\0';

  return 0; // Success
}

