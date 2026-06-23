# Concurrent HTTP Server (C++)

A multi-threaded HTTP server built from scratch in C++17 using POSIX sockets, a thread-pool architecture, HTTP request parsing, persistent connections, static file serving, and configurable routing.

The project demonstrates systems programming concepts including network programming, concurrency, synchronization, request processing, performance benchmarking, and security-focused request validation.

## Features

### Core Functionality

* HTTP/1.0 and HTTP/1.1 request handling
* Persistent connections (Keep-Alive)
* Static file serving
* GET and POST routing
* MIME type detection
* Configurable server settings

### Concurrency

* Fixed-size worker thread pool
* Thread-safe task queue
* Producer-consumer architecture
* Concurrent client handling

### Reliability & Security

* Request validation
* Header size limits
* Header count limits
* Directory traversal protection
* Request timeout handling
* Graceful shutdown support

### Observability

* Thread-safe logging system
* Request metrics collection
* Benchmarking and performance analysis

## Architecture

```text
Client
   │
TCP Socket
   │
Accept Loop
   │
Connection Queue
   │
Thread Pool
   │
Request Handler
   ├── Router
   ├── Static File Server
   ├── Logger
   └── Metrics
```

## Project Structure

```text
src/
├── config/
├── http/
├── logger/
├── router/
├── threadpool/
└── server.cpp

tests/
├── config_test.cpp
├── response_test.cpp
└── router_test.cpp
```

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

## Build

```bash
mkdir build
cd build

cmake ..
make
```

## Run

```bash
./server
```

## Testing

Run the test suite:

```bash
ctest --output-on-failure
```

Current tests cover:

* Route registration and dispatch
* Configuration parsing
* MIME type handling

## Benchmark Summary

| Workload       | Concurrent Clients | Throughput |
| -------------- | ------------------ | ---------- |
| 100 Requests   | 10                 | 1101 req/s |
| 5000 Requests  | 100                | 2676 req/s |
| 10000 Requests | 500                | 2551 req/s |
| 20000 Requests | 1000               | 2496 req/s |

The server sustained approximately **2500–2700 requests/second** under heavy concurrent load with zero failed requests.

Detailed benchmarking methodology and results are available in `benchmark.md`.

## Documentation

* `benchmark.md` – performance evaluation
* `security.md` – security analysis
* `optimization_report.md` – optimization work
* `report.md` – implementation details

```
```
