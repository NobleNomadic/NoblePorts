// parser.h - Header for parsing HTTP requests into structured data
#ifndef PARSER_H
#define PARSER_H

#define MAX_METHOD_LEN 8
#define MAX_PATH_LEN 256

typedef struct {
  char method[MAX_METHOD_LEN];
  char path[MAX_PATH_LEN];
} HTTPRequest;

int parseHTTPRequest(const char *rawRequest, HTTPRequest *request);

#endif // PARSER_H

