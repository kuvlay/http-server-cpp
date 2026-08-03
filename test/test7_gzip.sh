#!/bin/bash

echo "=============================="
echo "Test 1: No Accept-Encoding"
echo "Expected:"
echo "- 200 OK"
echo "- NO Content-Encoding header"
echo "- Body = abc"
echo "=============================="
curl -i http://localhost:4221/echo/abc

echo -e "\n\n=============================="
echo "Test 2: Accept-Encoding: gzip"
echo "Expected:"
echo "- 200 OK"
echo "- Content-Encoding: gzip"
echo "- Binary body"
echo "=============================="
curl -i \
-H "Accept-Encoding: gzip" \
http://localhost:4221/echo/abc

echo -e "\n\n=============================="
echo "Test 3: Multiple encodings"
echo "Expected:"
echo "- 200 OK"
echo "- Content-Encoding: gzip"
echo "=============================="
curl -i \
-H "Accept-Encoding: br, gzip, deflate" \
http://localhost:4221/echo/abc

echo -e "\n\n=============================="
echo "Test 4: Invalid encoding"
echo "Expected:"
echo "- 200 OK"
echo "- NO Content-Encoding header"
echo "=============================="
curl -i \
-H "Accept-Encoding: invalid-encoding" \
http://localhost:4221/echo/abc

echo -e "\n\n=============================="
echo "Test 5: Hexdump of compressed response"
echo "Expected:"
echo "- Starts with: 1f 8b 08"
echo "=============================="
curl \
-H "Accept-Encoding: gzip" \
http://localhost:4221/echo/abc | hexdump -C

echo -e "\n\n=============================="
echo "Test 6: Reference gzip output"
echo "Expected:"
echo "- Should closely match Test 5"
echo "=============================="
echo -n "abc" | gzip | hexdump -C

echo -e "\n\n=============================="
echo "Test 7: Longer string"
echo "Expected:"
echo "- Content-Encoding: gzip"
echo "- Correct binary response"
echo "=============================="
curl -i \
-H "Accept-Encoding: gzip" \
http://localhost:4221/echo/ThisIsALongerStringToCompress123456789

echo -e "\n\n=============================="
echo "All gzip tests completed."
echo "=============================="