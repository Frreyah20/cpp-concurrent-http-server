# Benchmark Report

## Environment

| Parameter      | Value                      |
| -------------- | -------------------------- |
| Server Type    | Multi-threaded HTTP Server |
| Language       | C++17                      |
| Benchmark Tool | ApacheBench (ab)           |
| OS             | Linux / WSL                |
| Port           | 8081                       |

---

## Throughput and Latency

### Test 1

| Metric             | Value   |
| ------------------ | ------- |
| Requests           | 100     |
| Concurrent Clients | 10      |
| Requests/sec       | 1101.36 |
| P50 Latency        | 6 ms    |
| P95 Latency        | 20 ms   |
| P99 Latency        | 25 ms   |
| Failed Requests    | 0       |

### Test 2

| Metric             | Value   |
| ------------------ | ------- |
| Requests           | 5000    |
| Concurrent Clients | 100     |
| Requests/sec       | 2676.36 |
| P50 Latency        | 34 ms   |
| P95 Latency        | 39 ms   |
| P99 Latency        | 73 ms   |
| Failed Requests    | 0       |

### Test 3

| Metric             | Value   |
| ------------------ | ------- |
| Requests           | 10000   |
| Concurrent Clients | 500     |
| Requests/sec       | 2551.49 |
| P50 Latency        | 197 ms  |
| P95 Latency        | 213 ms  |
| P99 Latency        | 219 ms  |
| Failed Requests    | 0       |

### Test 4

| Metric             | Value   |
| ------------------ | ------- |
| Requests           | 20000   |
| Concurrent Clients | 1000    |
| Requests/sec       | 2495.80 |
| P50 Latency        | 397 ms  |
| P95 Latency        | 442 ms  |
| P99 Latency        | 454 ms  |
| Failed Requests    | 0       |

---

## Keep-Alive Evaluation

### Benchmark Command

```bash
ab -k -n 20000 -c 100 http://127.0.0.1:8081/
```

### Results

| Metric          | Without Keep-Alive | With Keep-Alive |
| --------------- | -----------------: | --------------: |
| Requests/sec    |            1871.01 |         1930.49 |
| Mean Latency    |           53.45 ms |        51.80 ms |
| Transfer Rate   |        168.10 KB/s |     182.87 KB/s |
| Failed Requests |                  0 |               0 |

### Improvement

| Metric        | Change |
| ------------- | ------ |
| Throughput    | +3.18% |
| Mean Latency  | -3.08% |
| Transfer Rate | +8.79% |

---

## Key Findings

* Sustained approximately 2500–2700 requests/second under heavy concurrent load.
* Successfully handled up to 20,000 requests with 1,000 concurrent clients and zero failed requests.
* Increasing the TCP listen backlog from 5 to 1024 eliminated connection timeouts under high concurrency.
* Keep-Alive connections improved throughput and reduced latency by reducing TCP connection establishment overhead.
* Latency increased with concurrency due to queueing effects within the thread-pool architecture.

## Limitations

The server uses a thread-per-task worker-pool design. Under very high concurrency, persistent connections can occupy worker threads and reduce scalability.

Production-grade web servers typically use event-driven I/O mechanisms such as:

* epoll
* kqueue
* io_uring

to manage large numbers of concurrent connections more efficiently.

```
```
