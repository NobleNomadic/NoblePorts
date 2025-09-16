#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../header/socket.h"

int main (int argc, char **argv) {
  // Argument fail
  if (argc < 3) {
    fprintf(stderr, "Usage: %s <Port> <HTTP|HTTPS>\n", argv[0]);
    exit(1);
  }

  // Extract args
  int port = atoi(argv[1]);
  int SSLMode = 0;
  if (strcmp(argv[2], "HTTPS") == 0) {
    SSLMode = 1;
  }

  // Start main server loop
  // HTTPS mode
  if (SSLMode == 1) {
    return 0;
  }
  // Raw mode
  else {
    return 0;
  }
}
