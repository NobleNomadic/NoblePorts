// socket.h - Socket handling simplification
#ifndef SOCKET_H
#define SOCKET_H

#include <stddef.h>

int newServerSocket(int port);
int acceptClientConnection(int serverSocket);

int receiveData(int socket, char *buffer, size_t receiveSize);
int sendData(int socket, const char *data);

#endif // SOCKET_H
