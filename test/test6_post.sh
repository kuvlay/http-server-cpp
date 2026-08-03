#!/bin/bash

BASE_DIR="/tmp/http-files"
BASE_URL="http://localhost:4221"

mkdir -p "$BASE_DIR"

echo "===== POST: hello.txt ====="
curl -i \
  -X POST \
  -H "Content-Type: application/octet-stream" \
  --data "Hello World" \
  "$BASE_URL/files/hello.txt"

echo
echo "===== GET: hello.txt ====="
curl -i "$BASE_URL/files/hello.txt"

echo
echo "===== POST: overwrite ====="
curl -i \
  -X POST \
  -H "Content-Type: application/octet-stream" \
  --data "ABCDEF" \
  "$BASE_URL/files/hello.txt"

echo
echo "===== GET after overwrite ====="
curl -i "$BASE_URL/files/hello.txt"

echo
echo "===== POST: empty ====="
curl -i \
  -X POST \
  -H "Content-Type: application/octet-stream" \
  --data "" \
  "$BASE_URL/files/empty.txt"

echo
echo "===== GET: empty ====="
curl -i "$BASE_URL/files/empty.txt"

echo
echo "===== Files Created ====="
ls -l "$BASE_DIR"