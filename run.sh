#!/bin/bash

g++ -Isrc \
	src/main.cpp \
	src/http/http.cpp \
	src/file/file.cpp \
	src/gzip/gzip.cpp \
	src/utils/utils.cpp \
	-o server -pthread -lz || exit 1

./server --directory /tmp/http-files/