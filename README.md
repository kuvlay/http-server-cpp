# C++ TLS HTTP / WebSocket Server

A C++ networking project implementing HTTP/1.1 and WebSocket handling directly over POSIX sockets and OpenSSL. It demonstrates how connection acceptance, TLS, request buffering, routing, and WebSocket frames fit together.

## Implementation

- Worker thread per client with an OpenSSL TLS handshake.
- Buffered HTTP requests, `Content-Length` body extraction, persistent connections, and pipelining.
- Root, echo, user-agent, file download, and file upload routes.
- Optional gzip compression for echo responses using zlib.
- WebSocket upgrade handshake, text echo, ping/pong, close handling, and extended payload lengths.
- Read/write loops that handle partial application data.

`Client → POSIX socket → OpenSSL TLS → HTTP parser → route or WebSocket handler`

## Build and run

Requirements: Linux or WSL, a C++17 compiler, OpenSSL development headers/libraries, zlib development headers/libraries, and the OpenSSL command-line tool.

```bash
git clone https://github.com/kuvlay/http-server-cpp.git
cd http-server-cpp
mkdir -p certs /tmp/http-files
openssl req -x509 -newkey rsa:2048 -nodes \
  -keyout certs/server.key -out certs/server.crt \
  -days 365 -subj "/CN=localhost"
g++ -std=c++17 -Isrc \
  src/main.cpp src/http/http.cpp src/http/routes.cpp \
  src/file/file.cpp src/gzip/gzip.cpp src/socket/websocket.cpp \
  -o server -pthread -lz -lssl -lcrypto
./server --directory /tmp/http-files/
```

The source expects `certs/server.crt` and `certs/server.key` relative to the repository root. These files are not included; generate local development certificates as above and keep the private key out of Git. The server listens on port 4221.

## Try the routes

The development certificate is self-signed, so local examples use `curl -k`:

```bash
curl -k -i https://localhost:4221/
curl -k -i https://localhost:4221/echo/hello
curl -k -i -H 'Accept-Encoding: gzip' https://localhost:4221/echo/hello
curl -k -i -X POST --data 'Hello World' https://localhost:4221/files/hello.txt
curl -k -i https://localhost:4221/files/hello.txt
```

WebSocket clients should connect with `wss://localhost:4221`. The repository includes HTTP shell checks and Python/WebSocket experiments under `test/`; some historical scripts target plaintext HTTP and need the TLS settings above. No test pass or performance result is implied by their presence.

## Scope and limitations

This is a local learning project. Its HTTP parser is basic, uses case-sensitive header lookups, relies on `Content-Length`, and does not implement chunked encoding, HTTP/2, or HTTP/3. WebSocket binary frames are not processed as application messages. File paths are concatenated directly without production-grade validation, so run only with trusted local requests and a disposable file directory. The server has no authentication, resource limits, or production hardening.

## Provenance

Imported from [aarish-io/http-server-cpp](https://github.com/aarish-io/http-server-cpp) at the source owner's request for the [kuvlay portfolio](https://github.com/kuvlay). Original history and contributor attribution are retained. No new license is asserted.
