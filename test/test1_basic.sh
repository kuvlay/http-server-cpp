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