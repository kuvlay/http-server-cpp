#!/bin/bash

g++ src/main.cpp -o server || exit 1

./server --directory /tmp/http-files/