// socket.c - Functions for handling plain TCP sockets (non-SSL/HTTPS)
#include "socket.h"

#include <stdio.h>        // For printf, perror
#include <stdlib.h>       // For exit
#include <string.h>       // For memset, strlen
#include <sys/socket.h>   // For socket functions
#include <sys/types.h>    // For data types
#include <netinet/in.h>   // For sockaddr_in
#include <unistd.h>       // For close

#define BACKLOG 10        // Number of pending connections queue will hold

// Create and return a new TCP server socket bound to the specified port.
int rawNewServerSocket(int port) {
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

// Accept a new TCP client connection and return the client socket file descriptor.
int rawAcceptClientConnection(int serverSocket) {
  // Define structure to hold client address information
  struct sockaddr_in clientAddr;
  socklen_t addrLen = sizeof(clientAddr);

  // Accept the incoming connection
  int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &addrLen);
  if (clientSocket < 0) {
    perror("Error accepting client connection");
    return -1;
  }

  // Return the client socket file descriptor
  return clientSocket;
}

// Receive data from the specified TCP client socket and store it in the buffer.
// Returns the number of bytes received, or -1 on failure.
int rawReceiveData(int clientSocket, char *buffer, size_t receiveSize) {
  // Clear the buffer before receiving data
  memset(buffer, 0, receiveSize);

  // Receive data over TCP connection
  int bytesReceived = recv(clientSocket, buffer, receiveSize, 0);
  if (bytesReceived < 0) {
    perror("Error receiving data");
    return -1;
  }

  // Return number of bytes successfully received
  return bytesReceived;
}

// Send a string of data through the specified TCP client socket.
// Returns the number of bytes sent, or -1 on failure.
int rawSendData(int clientSocket, const char *data) {
  // Get the length of the data to be sent
  int dataLen = strlen(data);

  // Send the data over TCP connection
  int bytesSent = send(clientSocket, data, dataLen, 0);
  if (bytesSent < 0) {
    perror("Error sending data");
    return -1;
  }

  // Return number of bytes successfully sent
  return bytesSent;
}

// Close a TCP socket (server or client).
void rawCloseSocket(int sock) {
  close(sock);
}

