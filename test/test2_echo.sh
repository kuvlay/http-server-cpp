#!/bin/bash

echo "===== Root ====="
curl -i http://localhost:4221/

echo
echo "===== Hello ====="
curl -i http://localhost:4221/hello

echo
echo "===== Random ====="
curl -i http://localhost:4221/abcdef

echo
echo "===== Another ====="
curl -i http://localhost:4221/test123

echo
echo "===== Echo abc ====="
curl -i http://localhost:4221/echo/abc

echo
echo "===== Echo hello ====="
curl -i http://localhost:4221/echo/hello

echo
echo "===== Echo CodeCrafters ====="
curl -i http://localhost:4221/echo/CodeCrafters

echo
echo "===== Echo Empty ====="
curl -i http://localhost:4221/echo/