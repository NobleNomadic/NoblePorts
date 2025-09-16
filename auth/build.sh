#!/bin/bash
set -e

gcc src/main/main.c src/header/socket.c -o build/auth

if [[ $1 == "run" ]]; then
  cd build
  ./auth 8090 HTTP
  cd ..
fi
