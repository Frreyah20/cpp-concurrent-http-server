# TCP Server Benchmark Report

## Environment

Workers: 2

Port: 8081

## Test 1

Requests: 100

Concurrent Clients: 10

Requests/sec: 1101.36

P50 Latency: 6 ms

P95 Latency: 20 ms

P99 Latency: 25 ms

Failed Requests: 0

## Test 2

Requests: 5000

Concurrent Clients: 100

Requests/sec: 2676.36

P50 Latency: 34 ms

P95 Latency: 39 ms

P99 Latency: 73 ms

Failed Requests: 0

## Test 3

Requests: 10000

Concurrent Clients: 500

Requests/sec: 2551.49

P50 Latency: 197 ms

P95 Latency: 213 ms

P99 Latency: 219 ms

Failed Requests: 0

Notes:
Increasing the listen backlog from 5 to 1024 eliminated connection timeouts under high concurrency.

# Test 4

Requests: 20000

Concurrent Clients: 1000

Requests/sec: 2495.80

P50 Latency: 397 ms

P95 Latency: 442 ms

P99 Latency: 454 ms

Failed Requests: 0

---

# Conclusions

The server sustained approximately 2500-2700 requests/second under heavy concurrent load.

Latency increased significantly as concurrency increased, indicating queueing effects within the thread pool architecture.

A major bottleneck was identified in the TCP listen backlog. Increasing the backlog from 5 to 1024 eliminated connection timeouts at 500 concurrent clients.

The server successfully handled 20,000 requests at 1000 concurrent clients with zero failed requests.

# Phase 13 — Keep-Alive Connections

## Objective

Implement HTTP persistent connections (`Connection: keep-alive`) to reduce the overhead of creating and closing TCP connections for every request.

## Implementation

The server was modified to:

* Parse the `Connection` header from incoming HTTP requests.
* Support persistent client connections using a request-processing loop.
* Keep the socket open when the client requests `keep-alive`.
* Close the connection only when:

  * the client requests `Connection: close`,
  * a timeout occurs,
  * or the client disconnects.

Example:

```http
Connection: keep-alive
```

## Benchmark Setup

Tool:

```bash
ApacheBench (ab)
```

Server Configuration:

```text
Workers: 100
Port: 8081
```

Test Endpoint:

```http
GET /
```

Response Body:

```text
Home Page
```

Response Size:

```text
9 bytes
```

---

## Benchmark Results

### Without Keep-Alive

Command:

```bash
ab -n 20000 -c 100 http://127.0.0.1:8081/
```

Results:

| Metric            | Value       |
| ----------------- | ----------- |
| Complete Requests | 20000       |
| Failed Requests   | 0           |
| Requests/sec      | 1871.01     |
| Mean Request Time | 53.447 ms   |
| Transfer Rate     | 168.10 KB/s |

---

### With Keep-Alive

Command:

```bash
ab -k -n 20000 -c 100 http://127.0.0.1:8081/
```

Results:

| Metric            | Value       |
| ----------------- | ----------- |
| Complete Requests | 20000       |
| Failed Requests   | 0           |
| Requests/sec      | 1930.49     |
| Mean Request Time | 51.800 ms   |
| Transfer Rate     | 182.87 KB/s |

---

## Performance Comparison

| Metric          | No Keep-Alive |  Keep-Alive | Improvement |
| --------------- | ------------: | ----------: | ----------: |
| Requests/sec    |       1871.01 |     1930.49 |      +3.18% |
| Mean Latency    |     53.447 ms |   51.800 ms |      -3.08% |
| Transfer Rate   |   168.10 KB/s | 182.87 KB/s |      +8.79% |
| Failed Requests |             0 |           0 |   No Change |

---

## Stress Testing

### Moderate Load

Command:

```bash
ab -k -n 1000 -c 100 http://127.0.0.1:8081/
```

Result:

```text
Complete Requests: 1000
Failed Requests: 0
```

### Single Persistent Connection

Command:

```bash
ab -k -n 1000 -c 1 http://127.0.0.1:8081/
```

Result:

```text
Complete Requests: 1000
Failed Requests: 0
```

### High Concurrency

Command:

```bash
ab -k -n 5000 -c 500 http://127.0.0.1:8081/
```

Result:

```text
Completed Requests: ~4600
```

Command:

```bash
ab -k -n 20000 -c 1000 http://127.0.0.1:8081/
```

Result:

```text
Completed Requests: ~19100
```

At very high concurrency levels, some requests timed out before completion.

---

## Analysis

Keep-Alive successfully reduced connection setup overhead and improved throughput by approximately 3%.

The improvement is relatively small because the benchmarked endpoint returns only a 9-byte response on localhost. Under these conditions, most execution time is spent in:

* thread scheduling,
* mutex synchronization,
* request parsing,
* logging,
* socket operations.

The cost of TCP connection establishment represents only a small portion of total request processing time.

---

## Current Limitations

The server uses a thread-pool architecture where each worker remains associated with a client connection while it is active.

Under very high concurrency, persistent connections can occupy worker threads for extended periods, reducing scalability.

Production-grade web servers typically use:

* epoll
* kqueue
* io_uring
* asynchronous event-driven architectures

to manage large numbers of persistent connections efficiently.

---

## Conclusion

Keep-Alive support was successfully implemented and validated through ApacheBench testing.

Results show:

* Reduced average latency
* Increased throughput
* Correct handling of persistent HTTP connections
* Stable operation under moderate concurrent workloads

The implementation meets the objectives of Phase 13 and provides measurable performance improvements over the non-persistent connection model.

