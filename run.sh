#!/bin/bash

g++ src/main.cpp -o server -pthread -lz || exit 1

./server --directory /tmp/http-files/