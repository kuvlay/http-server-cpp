#!/bin/bash

BASE_DIR="/tmp/http-files"
BASE_URL="http://localhost:4221"

echo "===== Preparing Test Files ====="

mkdir -p "$BASE_DIR"

echo -n "Hello, World!" > "$BASE_DIR/foo"

echo -n "CodeCrafters HTTP Server" > "$BASE_DIR/bar"

echo
echo "===== Existing File: foo ====="
curl -i "$BASE_URL/files/foo"

echo
echo "===== Existing File: bar ====="
curl -i "$BASE_URL/files/bar"

echo
echo "===== Empty File ====="
touch "$BASE_DIR/empty"
curl -i "$BASE_URL/files/empty"

echo
echo "===== Missing File ====="
curl -i "$BASE_URL/files/does_not_exist"

echo
echo "===== Directory Listing ====="
ls -l "$BASE_DIR"