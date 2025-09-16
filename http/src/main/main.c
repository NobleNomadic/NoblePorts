// main.c - Noble HTTP/HTTPS Server Entry Point

#include <stdio.h>         // For printf, perror
#include <stdlib.h>        // For exit, atoi, malloc, free
#include <string.h>        // For memset, strlen
#include <unistd.h>        // For close
#include <fcntl.h>         // For file I/O

// Include custom headers
#include "../header/sslsocket.h"   // TLS socket functions for HTTPS
#include "../header/socket.h"      // Regular raw socket functions for HTTP
#include "../header/parser.h"      // HTTP request parsing

// ==== FUNCTION: readFile ====
// Read contents of a file and return it as a dynamically allocated string.
// Caller is responsible for freeing the memory.
// If the file does not exist or cannot be opened, return NULL.
char *readFile(const char *filePath) {
  FILE *file = fopen(filePath, "r");
  if (!file) {
    printf("[!] Failed to open file: %s\n", filePath);  // Output error
    return NULL;
  }

  // Seek to the end to determine file size
  fseek(file, 0, SEEK_END);
  long fileSize = ftell(file);
  rewind(file);  // Reset to beginning

  // Allocate buffer to hold the entire file (+1 for null terminator)
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

// ==== FUNCTION: HandleClient ====
// Handle a single client session.
// Reads request, parses it, and sends appropriate HTTP response.
void HandleClient(void *connection, int isSSL) {
  // Step 1: Create buffer for raw HTTP request
  char requestBuffer[2048];
  memset(requestBuffer, 0, sizeof(requestBuffer));  // Zero out buffer

  // Step 2: Receive the HTTP request from the client
  if (isSSL) {
    SSLReceiveData((SSL *)connection, requestBuffer, sizeof(requestBuffer));  // SSL version
  } else {
    rawReceiveData(*(int *)connection, requestBuffer, sizeof(requestBuffer));  // Raw HTTP version
  }
  printf("[*] Received request\n%s\n", requestBuffer);

  // Step 3: Parse raw request into structured format
  HTTPRequest requestStructure;
  if (parseHTTPRequest(requestBuffer, &requestStructure) != 0) {
    // Parsing failed, send 400 Bad Request response
    const char *badRequestResponse =
      "HTTP/1.1 400 Bad Request\r\n\r\nMalformed HTTP request.";
    if (isSSL) {
      SSLSendData((SSL *)connection, badRequestResponse);
    } else {
      rawSendData(*(int *)connection, badRequestResponse);
    }
    return;
  }

  // Step 4: Verify that the HTTP method is supported (only GET)
  if (strcmp(requestStructure.method, "GET") != 0) {
    // Unsupported method — send 405 Method Not Allowed
    const char *methodNotAllowed =
      "HTTP/1.1 405 Method Not Allowed\r\n\r\nOnly GET is allowed.";
    if (isSSL) {
      SSLSendData((SSL *)connection, methodNotAllowed);
    } else {
      rawSendData(*(int *)connection, methodNotAllowed);
    }
    return;
  }

  // Step 5: Determine the requested file path
  char *requestedPath = requestStructure.path[0] == '/'
    ? requestStructure.path + 1  // Skip leading slash
    : requestStructure.path;

  // If no path is provided, default to "index.html"
  if (strlen(requestedPath) == 0) {
    requestedPath = "index.html";
  }

  // Step 6: Security check — block access to subdirectories
  if (strchr(requestedPath, '/')) {
    const char *forbiddenResponse =
      "HTTP/1.1 403 Forbidden\r\n\r\nAccess to subdirectories is not allowed.";
    if (isSSL) {
      SSLSendData((SSL *)connection, forbiddenResponse);
    } else {
      rawSendData(*(int *)connection, forbiddenResponse);
    }
    return;
  }

  // Step 7: Build full file path from request
  char fullPath[512];
  snprintf(fullPath, sizeof(fullPath), "www/%s", requestedPath);

  // Step 8: Attempt to read the requested file from disk
  char *fileContent = readFile(fullPath);
  if (!fileContent) {
    // File not found — send 404 response
    const char *notFound =
      "HTTP/1.1 404 Not Found\r\n\r\nFile not found.";
    if (isSSL) {
      SSLSendData((SSL *)connection, notFound);
    } else {
      rawSendData(*(int *)connection, notFound);
    }
    return;
  }

  // Step 9: Construct HTTP response header
  char responseHeader[256];
  snprintf(responseHeader, sizeof(responseHeader),
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Content-Length: %lu\r\n"
    "\r\n", strlen(fileContent));

  // Step 10: Send response header followed by file content
  if (isSSL) {
    SSLSendData((SSL *)connection, responseHeader);
    SSLSendData((SSL *)connection, fileContent);
  } else {
    rawSendData(*(int *)connection, responseHeader);
    rawSendData(*(int *)connection, fileContent);
  }

  // Step 11: Clean up allocated memory
  free(fileContent);
  return;
}

void SSLServerLoop(int port) {
  // Initialize TLS context and load certificates
  printf("[*] Initializing SSL context\n");
  SSL_CTX *sslContext = initTLSContext();
  loadCertificates(sslContext, "cert.pem", "key.pem");  // Load cert and key

  // Create the main server socket
  printf("[*] Creating new server socket on port %d\n", port);
  int serverSocketFD = newServerSocket(port);
  if (serverSocketFD < 0) {
    fprintf(stderr, "[!] Failed to create server socket\n");
    return;
  }

  // Main server loop for handling a client socket
  while (1) {
    printf("[*] Waiting for HTTPS connection on port %d\n", port);

    // Accept a new client connection and establish a TLS session
    SSL *ssl = acceptClientConnection(serverSocketFD, sslContext);
    if (!ssl) {
      fprintf(stderr, "[!] TLS handshake failed or client connection error\n");
      continue;  // Skip to next connection
    }

    printf("[+] Client connected via TLS\n");

    // Handle the request and send a response
    HandleClient(ssl, 1);  // '1' indicates SSL connection

    // Shutdown SSL connection
    printf("[*] Closing client connection\n");
    SSL_shutdown(ssl);
    int clientFD = SSL_get_fd(ssl);  // Get underlying socket FD
    SSL_free(ssl);                   // Free SSL structure
    close(clientFD);                 // Close socket
  }
}

void HTTPServerLoop(int port) {
  // Create main server socket
  printf("[*] Creating server socket\n");
  int serverSocketFD = rawNewServerSocket(port);

  // Main loop for accepting clients
  while (1) {
    printf("[*] Waiting for connection on port %d\n", port);

    // Accept a new client
    int clientSocketFD = rawAcceptClientConnection(serverSocketFD);
    printf("[+] Client connected\n");

    // Handle the client request
    HandleClient(&clientSocketFD, 0);  // '0' indicates raw HTTP connection

    // Finish
    printf("[*] Closing client connection");
    rawCloseSocket(clientSocketFD);
  }
}

// ==== FUNCTION: main ====
// Entry point for the Noble HTTP/HTTPS server.
int main(int argc, char **argv) {
  printf("NOBLE PORTS HTTP SERVER 0.2.0\n");

  // Argument failure
  if (argc < 3) {
    fprintf(stderr, "Usage: ./%s <Port> <HTTPS|HTTP>\n", argv[0]);
    return 1;  // Incorrect usage
  }

  // Extract port number from arguments
  int port = atoi(argv[1]);
  int SSLMode = 0;
  // If HTTPS argument was used, use SSL HTTPS sockets
  if (strcmp(argv[2], "HTTPS") == 0) {
    SSLMode = 1;
  }

  if (SSLMode == 1) {
    SSLServerLoop(port);
  } else {
    HTTPServerLoop(port);
  }

  return 0;
}

