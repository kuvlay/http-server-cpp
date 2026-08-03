#!/bin/bash

echo "========================================"
echo "Test 1: Single persistent connection"
echo "========================================"

curl --http1.1 -v \
http://localhost:4221/user-agent \
-H "User-Agent: orange/mango-grape" \
--next \
http://localhost:4221/echo/apple

echo -e "\n\n========================================"
echo "Test 2: Two concurrent persistent connections"
echo "========================================"

(
curl --http1.1 -v \
http://localhost:4221/user-agent \
-H "User-Agent: orange/mango-grape" \
--next \
http://localhost:4221/echo/apple
) &

(
curl --http1.1 -v \
http://localhost:4221/echo/banana \
--next \
http://localhost:4221/user-agent \
-H "User-Agent: blueberry/apple"
) &

wait

echo -e "\n\n========================================"
echo "Test 3: Three concurrent clients"
echo "========================================"

(
curl --http1.1 \
http://localhost:4221/echo/client1 \
--next \
http://localhost:4221/user-agent \
-H "User-Agent: client1"
) &

(
curl --http1.1 \
http://localhost:4221/echo/client2 \
--next \
http://localhost:4221/user-agent \
-H "User-Agent: client2"
) &

(
curl --http1.1 \
http://localhost:4221/echo/client3 \
--next \
http://localhost:4221/user-agent \
-H "User-Agent: client3"
) &

wait

echo -e "\n\n========================================"
echo "All persistent connection tests finished."
echo "========================================"