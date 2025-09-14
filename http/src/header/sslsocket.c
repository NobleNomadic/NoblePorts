// sslsocket.c - Functions for handling SSL sockets for HTTPS
#include "sslsocket.h"

#include <stdio.h>        // For printf, perror
#include <stdlib.h>       // For exit, malloc, free
#include <string.h>       // For memset, strlen
#include <sys/socket.h>   // For socket functions
#include <sys/types.h>    // For data types
#include <netinet/in.h>   // For sockaddr_in
#include <unistd.h>       // For close
#include <openssl/ssl.h>  // For SSL/TLS support
#include <openssl/err.h>  // For SSL error reporting

#define BACKLOG 10        // Number of pending connections queue will hold

// Initializes the OpenSSL library and creates a new SSL context
// Returns a pointer to the initialized SSL_CTX structure
SSL_CTX *initTLSContext() {
  // Load all available encryption algorithms and error strings
  SSL_library_init();
  OpenSSL_add_all_algorithms();
  SSL_load_error_strings();

  // Create a new SSL context using the TLS server method
  const SSL_METHOD *method = TLS_server_method();
  SSL_CTX *ctx = SSL_CTX_new(method);
  if (!ctx) {
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
  }

  return ctx;
}

// Loads the certificate and private key into the SSL context.
void loadCertificates(SSL_CTX *ctx, const char *certFile, const char *keyFile) {
  // Load server certificate into SSL context
  if (SSL_CTX_use_certificate_file(ctx, certFile, SSL_FILETYPE_PEM) <= 0) {
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
  }

  // Load private key into SSL context
  if (SSL_CTX_use_PrivateKey_file(ctx, keyFile, SSL_FILETYPE_PEM) <= 0) {
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
  }

  // Verify that the private key matches the certificate
  if (!SSL_CTX_check_private_key(ctx)) {
    fprintf(stderr, "Private key does not match the certificate\n");
    exit(EXIT_FAILURE);
  }
}

// Create and return a new TCP server socket bound to the specified port.
int newServerSocket(int port) {
  // Create a new TCP socket (IPv4)
  int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (serverSocket < 0) {
    perror("Error creating socket");
    return -1;
  }

  // Set socket options to allow reuse of address and port
  int opt = 1;
  if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("Error setting socket options");
    close(serverSocket);
    return -1;
  }

  // Define the server address structure
  struct sockaddr_in serverAddr;
  memset(&serverAddr, 0, sizeof(serverAddr));         // Zero out the structure
  serverAddr.sin_family = AF_INET;                    // Use IPv4
  serverAddr.sin_addr.s_addr = INADDR_ANY;            // Bind to any interface
  serverAddr.sin_port = htons(port);                  // Convert port to network byte order

  // Bind the socket to the address and port
  if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
    perror("Error binding socket");
    close(serverSocket);
    return -1;
  }

  // Mark the socket as passive, ready to accept incoming connections
  if (listen(serverSocket, BACKLOG) < 0) {
    perror("Error listening on socket");
    close(serverSocket);
    return -1;
  }

  // Return the server socket file descriptor
  return serverSocket;
}

// Establishes a TLS session and returns the SSL* on success, or NULL on failure.
SSL *acceptClientConnection(int serverSocket, SSL_CTX *ctx) {
  // Define structure to hold client address information
  struct sockaddr_in clientAddr;
  socklen_t addrLen = sizeof(clientAddr);

  // Accept the incoming connection
  int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &addrLen);
  if (clientSocket < 0) {
    perror("Error accepting client connection");
    return NULL;
  }

  // Create a new SSL object for the accepted connection
  SSL *ssl = SSL_new(ctx);
  SSL_set_fd(ssl, clientSocket);  // Bind SSL to the client's socket

  // Perform TLS handshake with the client
  if (SSL_accept(ssl) <= 0) {
    ERR_print_errors_fp(stderr);
    SSL_free(ssl);
    close(clientSocket);
    return NULL;
  }

  // Return the SSL object representing the TLS session
  return ssl;
}

// Receive data from the specified TLS session and store it in the buffer.
// Returns the number of bytes received, or -1 on failure.
int SSLReceiveData(SSL *ssl, char *buffer, size_t receiveSize) {
  // Clear the buffer before receiving data
  memset(buffer, 0, receiveSize);

  // Receive data over TLS connection
  int bytesReceived = SSL_read(ssl, buffer, receiveSize);
  if (bytesReceived < 0) {
    ERR_print_errors_fp(stderr);
    return -1;
  }

  // Return number of bytes successfully received
  return bytesReceived;
}

// Send a string of data through the specified TLS session.
int SSLSendData(SSL *ssl, const char *data) {
  // Get the length of the data to be sent
  int dataLen = strlen(data);

  // Send the data over TLS connection
  int bytesSent = SSL_write(ssl, data, dataLen);
  if (bytesSent < 0) {
    ERR_print_errors_fp(stderr);
    return -1;
  }

  // Return number of bytes successfully sent
  return bytesSent;
}

