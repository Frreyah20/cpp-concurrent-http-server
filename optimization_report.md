# Profiling & Optimization Report

## Baseline Performance

Throughput: 2495 req/s

P50: 397 ms

P95: 442 ms

P99: 454 ms

## Memory Profiling

Tool: Valgrind

Definitely Lost: 0 bytes

Indirectly Lost: 0 bytes

Possibly Lost: 1216 bytes

Still Reachable: 84668 bytes

Observations:

No definite memory leaks were detected.

## CPU Profiling

(To be completed)

## Lock Contention Analysis

(To be completed)

## Optimizations Applied

(To be completed)

## Before vs After

(To be completed)

## CPU Profiling

Tool: top -H

Benchmark:
ab -n 100000 -c 1000 http://127.0.0.1:8081/

Observations:

- Worker threads utilized approximately 20-40% CPU each.
- System remained ~74% idle during benchmark execution.
- CPU resources were not saturated.

Conclusion:

The server is not CPU-bound. Performance bottlenecks are likely related to synchronization, socket I/O, or request handling overhead rather than raw computation.

## Optimization 1: Remove Request-Path Console Logging

Issue:

Every request generated multiple std::cout operations for headers, metrics, request information, and debugging output.

Observation:

Console output introduces locking and synchronization overhead. Under high concurrency, worker threads compete for access to stdout.

Change:

Removed verbose request-path logging and retained only essential logging.

Results (1000 concurrent clients):

Before:
P50 = 402 ms
P95 = 434 ms
P99 = 455 ms

After:
P50 = 209 ms
P95 = 237 ms
P99 = 241 ms

Impact:

Approximately 45–50% reduction in latency across all percentiles.

Conclusion:

Console I/O was a significant bottleneck under high concurrency.

## Optimization 2: Replace Metrics Counters with Atomics

Change:

- Converted requests_served, active_connections, and errors from int to std::atomic<int>.
- Removed several mutex acquisitions around simple counter updates.

Results:

Before:
P50 = 209 ms
P95 = 237 ms
P99 = 241 ms

After:
P50 = 216 ms
P95 = 264 ms
P99 = 280 ms

Conclusion:

The optimization produced no measurable improvement.
The server is not significantly limited by metrics counter synchronization.
The primary bottleneck lies elsewhere, likely networking and request handling overhead.

Attempted perf-based profiling, but the profiling package was unavailable in the WSL environment. CPU analysis was performed using top -H.

# Graceful Shutdown

Implemented SIGINT (Ctrl+C) handling.

Features:
- Stops accepting new connections.
- Closes listening socket.
- Wakes sleeping worker threads.
- Allows active requests to complete.
- Joins all worker threads before exit.

Shutdown Flow:

Ctrl+C
→ Signal Handler
→ Stop Accept Loop
→ Close Socket
→ Notify Workers
→ Join Threads
→ Clean Exit