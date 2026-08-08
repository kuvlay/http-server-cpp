#!/bin/bash

g++ -Isrc \
	src/main.cpp \
	src/http/http.cpp \
	src/http/routes.cpp \
	src/file/file.cpp \
	src/gzip/gzip.cpp \
	src/socket/websocket.cpp \
	-o server -pthread -lz -lcrypto || exit 1

./server --directory /tmp/http-files/