#!/bin/bash
set -e

gcc src/main/main.c src/header/sslsocket.c src/header/socket.c src/header/parser.c -lssl -lcrypto -o build/http

if [[ $1 == "run" ]]; then
  cd build
  ./http 8080 HTTP
  cd ..
fi
