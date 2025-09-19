// main.c - Main authentication server

#include <stdio.h>        // printf, fprintf
#include <stdlib.h>       // exit, atoi
#include <string.h>       // memset, strtok, strcmp, strncpy
#include <unistd.h>       // close
#include <openssl/sha.h>  // SHA256_DIGEST_LENGTH
#include <sqlite3.h>      // SQLite3

#include "../header/sslsocket.h"   // TLS socket functions
#include "../header/socket.h"      // Raw socket functions

// Global database handle
sqlite3 *db;

// Opens the database file and ensures the "users" table exists.
int initDatabase(const char *dbPath) {
  int rc = sqlite3_open(dbPath, &db);
  if (rc) {
    fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
    return 1;
  }

  const char *sql =
    "CREATE TABLE IF NOT EXISTS users ("
    "username TEXT PRIMARY KEY, "
    "password_hash TEXT NOT NULL);";

  char *errMsg = NULL;
  rc = sqlite3_exec(db, sql, 0, 0, &errMsg);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", errMsg);
    sqlite3_free(errMsg);
    return 1;
  }

  return 0;
}

// Fetch stored hash for a username. Returns 1 if found.
int getUserHash(const char *username, char *outputBuffer, size_t bufferSize) {
  const char *sql = "SELECT password_hash FROM users WHERE username = ?;";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
    return 0;
  }

  sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

  int rc = sqlite3_step(stmt);
  if (rc == SQLITE_ROW) {
    const unsigned char *hash = sqlite3_column_text(stmt, 0);
    strncpy(outputBuffer, (const char *)hash, bufferSize - 1);
    outputBuffer[bufferSize - 1] = '\0';
    sqlite3_finalize(stmt);
    return 1;
  }

  sqlite3_finalize(stmt);
  return 0;
}

// Reads "username hash" from client, checks DB, responds true/false.
void HandleClient(void *connection, int isSSL) {
  char buffer[2048];
  memset(buffer, 0, sizeof(buffer));

  int bytesRead = 0;
  if (isSSL) {
    bytesRead = SSLReceiveData((SSL *)connection, buffer, sizeof(buffer) - 1);
  } else {
    bytesRead = rawReceiveData(*(int *)connection, buffer, sizeof(buffer) - 1);
  }
  if (bytesRead <= 0) return;

  buffer[bytesRead] = '\0';

  char *username = strtok(buffer, " ");
  char *receivedHash = strtok(NULL, " ");

  if (!username || !receivedHash) {
    const char *resp = "false\n";
    if (isSSL) SSLSendData((SSL *)connection, resp);
    else rawSendData(*(int *)connection, resp);
    return;
  }

  char storedHash[256];
  if (!getUserHash(username, storedHash, sizeof(storedHash))) {
    const char *resp = "false\n";
    if (isSSL) SSLSendData((SSL *)connection, resp);
    else rawSendData(*(int *)connection, resp);
    return;
  }

  int authSuccess = strcmp(receivedHash, storedHash) == 0;

  const char *resp = authSuccess ? "true\n" : "false\n";
  if (isSSL) SSLSendData((SSL *)connection, resp);
  else rawSendData(*(int *)connection, resp);
}

// Accepts TLS connections and handles them in a loop.
void SSLServerLoop(int port) {
  SSL_CTX *sslContext = initTLSContext();
  loadCertificates(sslContext, "cert.pem", "key.pem");

  int serverSocketFD = newServerSocket(port);
  if (serverSocketFD < 0) return;

  while (1) {
    SSL *ssl = acceptClientConnection(serverSocketFD, sslContext);
    if (!ssl) continue;
    HandleClient(ssl, 1);
    SSL_shutdown(ssl);
    int clientFD = SSL_get_fd(ssl);
    SSL_free(ssl);
    close(clientFD);
  }
}

// Accepts plaintext TCP connections and handles them in a loop.
void HTTPServerLoop(int port) {
  int serverSocketFD = rawNewServerSocket(port);
  if (serverSocketFD < 0) return;

  while (1) {
    int clientSocketFD = rawAcceptClientConnection(serverSocketFD);
    if (clientSocketFD < 0) continue;
    HandleClient(&clientSocketFD, 0);
    rawCloseSocket(clientSocketFD);
  }
}

// ==== ENTRY ====
int main(int argc, char **argv) {
  printf("NOBLE AUTHENTICATION SERVER 0.1.0\n");

  // Check argument failure
  if (argc < 3) {
    fprintf(stderr, "Usage: %s <Port> <HTTPS|HTTP>\n", argv[0]);
    return 1;
  }

  // Initialize the database
  if (initDatabase("users.db") != 0) {
    return 1;
  }

  // Extract arguments
  int port = atoi(argv[1]);
  int SSLMode = (strcmp(argv[2], "HTTPS") == 0);

  // Run SSL or raw HTTP mode
  if (SSLMode) SSLServerLoop(port);
  else HTTPServerLoop(port);
}

