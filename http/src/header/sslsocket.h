// sslsocket.h - HTTPS (TLS) socket handling simplification
#ifndef SSLSOCKET_H
#define SSLSOCKET_H

#include <stddef.h>
#include <openssl/ssl.h>

// Create and return a new TCP server socket bound to the specified port
int newServerSocket(int port);

// Initialize a new SSL context for the TLS server
SSL_CTX *initTLSContext(void);

// Load TLS certificate and private key into the context
void loadCertificates(SSL_CTX *ctx, const char *certFile, const char *keyFile);

// Accept a new TLS client connection and return an SSL session object
SSL *acceptClientConnection(int serverSocket, SSL_CTX *ctx);

// Receive data from a TLS session into the buffer
int SSLReceiveData(SSL *ssl, char *buffer, size_t receiveSize);

// Send data through a TLS session
int SSLSendData(SSL *ssl, const char *data);

#endif // SOCKET_H

