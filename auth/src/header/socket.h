// socket.h - Plain TCP socket handling simplification
#ifndef SOCKET_H
#define SOCKET_H

#include <stddef.h>

// Create and return a new TCP server socket bound to the specified port
int rawNewServerSocket(int port);

// Accept a new TCP client connection
int rawAcceptClientConnection(int serverSocket);

// Receive data from a TCP client socket into the buffer
int rawReceiveData(int clientSocket, char *buffer, size_t receiveSize);

// Send data through a TCP client socket
int rawSendData(int clientSocket, const char *data);

// Close a TCP socket
void rawCloseSocket(int sock);

#endif // SOCKET_H

