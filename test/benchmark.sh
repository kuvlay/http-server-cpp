#!/bin/bash

echo "===== ApacheBench ====="

ab -n 100 -c 10 http://localhost:4221/

echo
echo "----------------------------"

ab -n 1000 -c 50 http://localhost:4221/