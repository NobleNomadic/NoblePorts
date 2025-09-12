#!/bin/bash
set -e

gcc src/main/main.c src/header/socket.c src/header/parser.c -o build/http

if [[ $1 == "run" ]]; then
  cd build
  ./http 8080
  cd ..
fi
