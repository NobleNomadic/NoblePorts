#!/bin/bash
set -e

gcc src/main/main.c src/header/socket.c src/header/sslsocket.c -o build/auth -O2 -lssl -lcrypto -lsqlite3

if [[ $1 == "run" ]]; then
  cd build
  ./auth 8090 HTTP
  cd ..
fi
