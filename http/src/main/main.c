#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

// Include custom headers
#include "../header/socket.h"
#include "../header/parser.h"

// ==== FUNCTION: Read File ====
// Read contents of a file and return it as a dynamically allocated string.
// Caller is responsible for freeing the memory.
// If the file does not exist or cannot be opened, return NULL.
char *readFile(const char *filePath) {
  FILE *file = fopen(filePath, "r");
  if (!file) {
    printf("[!] Failed to open file: %s\n", filePath);  // Add debug output
    return NULL;  // File could not be opened
  }

  // Seek to the end to find file size
  fseek(file, 0, SEEK_END);
  long fileSize = ftell(file);
  rewind(file);  // Go back to the beginning

  // Allocate buffer (+1 for null terminator)
  char *buffer = malloc(fileSize + 1);
  if (!buffer) {
    fclose(file);
    return NULL;  // Memory allocation failed
  }

  // Read file into buffer
  fread(buffer, 1, fileSize, file);
  buffer[fileSize] = '\0';  // Null-terminate

  fclose(file);
  return buffer;
}

// ==== HANDLER FUNCTION ====
// Read request from a client socket, parse it, and send appropriate response.
void handleClient(int clientSocketFD) {
  // Step 1: Create buffer for raw HTTP request
  char requestBuffer[2048];
  memset(requestBuffer, 0, sizeof(requestBuffer));

  // Step 2: Receive the HTTP request from the client
  receiveData(clientSocketFD, requestBuffer, sizeof(requestBuffer));
  printf("[*] Received request\n%s\n", requestBuffer);

  // Step 3: Parse raw request into structured data
  HTTPRequest requestStructure;
  if (parseHTTPRequest(requestBuffer, &requestStructure) != 0) {
    // Failed to parse the request, return 400 Bad Request
    const char *badRequestResponse =
      "HTTP/1.1 400 Bad Request\r\n\r\nMalformed HTTP request.";
    sendData(clientSocketFD, badRequestResponse);
    return;
  }

  // Step 4: Check if the HTTP method is supported
  if (strcmp(requestStructure.method, "GET") != 0) {
    // Method not allowed (only GET is supported)
    const char *methodNotAllowed =
      "HTTP/1.1 405 Method Not Allowed\r\n\r\nOnly GET is allowed.";
    sendData(clientSocketFD, methodNotAllowed);
    return;
  }

  // Step 5: Sanitize path — default to "index.html" if no path provided
  char *requestedPath = requestStructure.path[0] == '/'
    ? requestStructure.path + 1
    : requestStructure.path;

  if (strlen(requestedPath) == 0) {
    requestedPath = "index.html";  // Default fallback file
  }

  // Step 6: Security Check — Block any paths containing slashes (e.g., subdirectories or traversal)
  if (strchr(requestedPath, '/')) {
    const char *forbiddenResponse =
      "HTTP/1.1 403 Forbidden\r\n\r\nAccess to subdirectories is not allowed.";
    sendData(clientSocketFD, forbiddenResponse);
    return;
  }

  // Optional: Prepend a known directory for serving (e.g., "www/")
  char fullPath[512];
  snprintf(fullPath, sizeof(fullPath), "www/%s", requestedPath);

  // Step 7: Read file content from disk
  char *fileContent = readFile(fullPath);
  if (!fileContent) {
    // File not found, send 404 response
    const char *notFound =
      "HTTP/1.1 404 Not Found\r\n\r\nFile not found.";
    sendData(clientSocketFD, notFound);
    return;
  }

  // Step 8: Create and send HTTP response with file content
  char responseHeader[256];
  snprintf(responseHeader, sizeof(responseHeader),
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Content-Length: %lu\r\n"
    "\r\n", strlen(fileContent));

  // Send headers first, then content
  sendData(clientSocketFD, responseHeader);
  sendData(clientSocketFD, fileContent);

  // Step 9: Clean up
  free(fileContent);
  return;
}

// ==== ENTRY ====
int main(int argc, char **argv) {
  printf("NOBLE HTTP SERVER 0.1.0\n");

  // Step 1: Check for correct argument usage
  if (argc < 2) {
    printf("Usage: ./%s <Port>\n", argv[0]);
    return 1;
  }

  // Step 2: Parse port number from arguments
  int port = atoi(argv[1]);

  // Step 3: Create new server socket
  printf("[*] Creating new server socket\n");
  int serverSocketFD = newServerSocket(port);

  // Step 4: Begin main server loop
  while (1) {
    printf("[*] Waiting for connection on port %d\n", port);

    // Accept a new client connection
    int clientSocketFD = acceptClientConnection(serverSocketFD);
    printf("[+] Client connected\n");

    // Handle client request
    handleClient(clientSocketFD);

    // Close client socket after handling
    close(clientSocketFD);
  }

  return 0;
}

