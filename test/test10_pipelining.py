import socket
import ssl

context = ssl.create_default_context()
context.check_hostname = False
context.verify_mode = ssl.CERT_NONE

sock = socket.create_connection(("localhost", 4221))
ssl_sock = context.wrap_socket(sock, server_hostname="localhost")

request = (
    "GET /echo/one HTTP/1.1\r\n"
    "Host: localhost:4221\r\n"
    "Connection: keep-alive\r\n"
    "\r\n"
    "GET /echo/two HTTP/1.1\r\n"
    "Host: localhost:4221\r\n"
    "Connection: close\r\n"
    "\r\n"
)

ssl_sock.sendall(request.encode())

while True:
    data = ssl_sock.recv(4096)

    if not data:
        break

    print(data.decode(errors="replace"))

ssl_sock.close()