# Concurrent HTTP Server

A high-performance multi-threaded HTTP server built in C++ using POSIX sockets, a thread pool architecture, persistent connections, routing, static file serving, logging, metrics collection, and security hardening.

The project was developed incrementally from a basic TCP socket server into a production-inspired HTTP server capable of handling concurrent clients while maintaining robust request validation and resource management.

---

## Features

### Networking

* TCP server built using POSIX sockets
* HTTP/1.0 and HTTP/1.1 support
* Persistent connections (`Keep-Alive`)
* Configurable listening port
* Graceful shutdown handling

### Concurrency

* Fixed-size worker thread pool
* Thread-safe task queue
* Producer-consumer architecture
* Concurrent request processing

### Routing

Built-in lightweight router supporting:

```http
GET /
GET /about
GET /contact
POST /api/data
```

### Static File Serving

Serves files from a configurable document root.

Supported file types:

* HTML
* CSS
* JavaScript
* PNG

### Logging

Thread-safe logging system with:

* INFO
* WARNING
* ERROR

Log output is written to:

```text
server.log
```

### Metrics

Tracks:

* Requests served
* Active connections
* Error count
* Average latency
* Throughput

### Security & Robustness

* Header size limits
* Header count limits
* Request validation
* HTTP version validation
* Method validation
* Request timeout handling
* Directory traversal protection
* Graceful connection error handling

---

## Architecture

```text
Client
  │
  ▼
Acceptor Thread
  │
  ▼
Task Queue
  │
  ▼
Worker Thread Pool
  │
  ▼
HTTP Request Parser
  │
  ▼
Router / Static File Handler
  │
  ▼
Response Generator
  │
  ▼
Client
```

### Request Lifecycle

1. Client establishes a TCP connection.
2. Acceptor thread accepts the socket.
3. Socket is pushed into the task queue.
4. Worker thread retrieves the task.
5. HTTP request is parsed and validated.
6. Router or static file handler processes the request.
7. HTTP response is generated and returned.
8. Connection is closed or kept alive based on request headers.

---

## Project Structure

```text
tcp_server/
├── src/
│   └── server.cpp
├── public/
│   ├── index.html
│   ├── about.html
│   ├── style.css
│   ├── app.js
│   └── test_img.png
├── build/
├── config.txt
├── benchmark.md
├── security.md
├── report.md
├── optimization_report.md
├── CMakeLists.txt
└── README.md
```

---

## Configuration

Configuration is loaded from:

```text
config.txt
```

Example:

```text
port=8081
workers=100
root=../public
```

| Parameter | Description               |
| --------- | ------------------------- |
| port      | Server listening port     |
| workers   | Number of worker threads  |
| root      | Static file document root |

---

## Build Instructions

### Requirements

* Linux / WSL
* CMake 3.10+
* C++17 compiler
* pthread support

### Build

```bash
mkdir build
cd build

cmake ..
make
```

### Run

```bash
./server
```

---

## Benchmark Results

Benchmarks performed using ApacheBench.

### Throughput & Latency

| Clients | Requests/sec | P95 Latency |
| ------- | -----------: | ----------: |
| 10      |         3013 |        6 ms |
| 100     |         2665 |       84 ms |
| 500     |         2859 |      243 ms |

### Keep-Alive Performance

| Metric       | Without Keep-Alive | With Keep-Alive |
| ------------ | -----------------: | --------------: |
| Requests/sec |               1871 |            1930 |
| Mean Latency |           53.45 ms |        51.80 ms |

Keep-Alive reduced TCP connection overhead and improved throughput under repeated request workloads.

Detailed benchmark results are available in:

```text
benchmark.md
```

---

## Security Features

Implemented protections include:

* Oversized header protection
* Header flooding protection
* Invalid request detection
* Unsupported method handling
* Request timeout handling
* Directory traversal prevention

Supported error responses:

| Status Code | Description                     |
| ----------- | ------------------------------- |
| 400         | Bad Request                     |
| 403         | Forbidden                       |
| 404         | Not Found                       |
| 405         | Method Not Allowed              |
| 408         | Request Timeout                 |
| 431         | Request Header Fields Too Large |

Additional details are available in:

```text
security.md
```

---

## Future Improvements

Potential extensions:

* epoll-based event loop
* Non-blocking sockets
* HTTPS/TLS support
* Request body parsing
* Rate limiting
* Access logging
* Zero-copy file serving (`sendfile`)
* WebSocket support

---

## Key Learning Outcomes

This project provided hands-on experience with:

* Network programming
* POSIX sockets
* Concurrent systems design
* Thread pools
* Synchronization primitives
* HTTP protocol internals
* Performance benchmarking
* Security hardening
* Systems-level debugging

---

## License

This project is intended for educational and portfolio purposes.
