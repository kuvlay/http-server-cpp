#!/bin/bash

BASE_URL="http://localhost:4221"

echo "==============================="
echo " HTTP Server Local Test Suite"
echo "==============================="

echo
echo "===== Root Route ====="
curl -i "$BASE_URL/"

echo
echo "===== Unknown Route ====="
curl -i "$BASE_URL/does-not-exist"

echo
echo "===== Echo Route ====="
curl -i "$BASE_URL/echo/hello"

echo
echo "===== Echo Empty ====="
curl -i "$BASE_URL/echo/"

echo
echo "===== User-Agent (Default curl) ====="
curl -i "$BASE_URL/user-agent"

echo
echo "===== User-Agent (Custom) ====="
curl -i \
    -H "User-Agent: AarishBrowser/1.0" \
    "$BASE_URL/user-agent"

echo
echo "===== User-Agent (Long Value) ====="
curl -i \
    -H "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 Chrome/140.0.0.0 Safari/537.36" \
    "$BASE_URL/user-agent"

echo
echo "==============================="
echo " Tests Complete"
echo "==============================="