import socket
import os
import base64

HOST = "127.0.0.1"
PORT = 4221

sock = socket.create_connection((HOST, PORT))

key = base64.b64encode(os.urandom(16)).decode()

handshake = (
    "GET / HTTP/1.1\r\n"
    "Host: localhost:4221\r\n"
    "Upgrade: websocket\r\n"
    "Connection: Upgrade\r\n"
    f"Sec-WebSocket-Key: {key}\r\n"
    "Sec-WebSocket-Version: 13\r\n"
    "\r\n"
)

sock.sendall(handshake.encode())

response = sock.recv(4096)

if b"101 Switching Protocols" not in response:
    print("FAIL: WebSocket handshake")
    sock.close()
    exit(1)

print("PASS: WebSocket handshake")

payload = b"hello"

mask_key = os.urandom(4)

masked_payload = bytes(
    payload[i] ^ mask_key[i % 4]
    for i in range(len(payload))
)

frame = bytes([
    0x89,
    0x80 | len(payload)
]) + mask_key + masked_payload

sock.sendall(frame)

header = sock.recv(2)

if len(header) != 2:
    print("FAIL: No Pong received")
    sock.close()
    exit(1)

opcode = header[0] & 0x0F
masked = (header[1] >> 7) & 1
length = header[1] & 0x7F

pong_payload = sock.recv(length)

if opcode == 0xA and masked == 0 and pong_payload == payload:
    print("PASS: Ping/Pong")
else:
    print("FAIL: Invalid Pong")

sock.close()