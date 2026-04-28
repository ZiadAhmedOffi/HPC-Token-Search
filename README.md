# Distributed String Search (HPC Project 2)

A high-performance, fault-tolerant distributed string matching application using a Master-Worker architecture over TCP/IP sockets.

## Prerequisites

- Docker and Docker Compose
- Python 3 (with pandas and matplotlib for visualization)
- g++ and make (for local building)

## Project Structure

- `src/`: C++ source code.
- `include/`: C++ header files.
- `docker/`: Dockerfile and docker-compose configuration.
- `scripts/`: Python scripts for benchmarking and visualization.
- `run_experiments.sh`: Script to automate the entire benchmarking process.

## Challenges and Solutions

### 1. Segmentation Faults (SEGV) during Connection
**Problem:** The Master would occasionally crash with a segmentation fault when trying to connect to workers.
**Cause:** 
- The code was dereferencing uninitialized or `NULL` pointers returned by `getaddrinfo` when a worker's hostname (e.g., `docker-worker-1`) wasn't yet resolvable by DNS.
- It was attempting to reuse the same socket file descriptor for multiple `connect()` calls after a failure, which is undefined behavior in POSIX and can lead to internal state corruption.
**Solution:**
- Added robust checks for `getaddrinfo` return codes and validated the `res` pointer before use.
- Implemented a "fresh socket" policy: the Master now closes and recreates a new socket for every connection attempt, ensuring a clean state for retries.

### 2. "No such file or directory" in Docker
**Problem:** Executing the binary inside the Alpine-based Docker container resulted in a "not found" error, even though the file existed.
**Cause:** 
- **Binary Incompatibility:** The binary was being built on a host (Ubuntu/glibc) and then mounted into an Alpine (musl) container. The dynamic loader expected by the binary didn't exist in Alpine.
- **Volume Overwrites:** The `docker-compose.yml` was mounting the host root directory over the container's `/app` directory, replacing the Alpine-compatible binary built during the Docker image creation with the incompatible host binary.
**Solution:**
- Updated the `Dockerfile` to install the binary to `/usr/local/bin` (part of the system PATH).
- Modified `docker-compose.yml` to mount the project root to `/data` and set it as the working directory, leaving the system `/app` and `/usr/local/bin` directories intact and protected.

### 3. Worker Connection Inconsistency
**Problem:** Sometimes workers would fail to accept connections or drop them unexpectedly.
**Cause:** 
- The `addrlen` variable in the worker's `accept()` loop was not being reset between iterations, leading to incorrect address size processing on subsequent connections.
**Solution:**
- Moved the `socklen_t current_addrlen = sizeof(address);` declaration inside the `while(true)` loop to ensure it is correctly initialized for every new connection.

## How to Run

### 1. Reset the environment
```bash
docker compose -f docker/docker-compose.yml down --rmi all
```

### 2. Build and Scale Workers
```bash
docker compose -f docker/docker-compose.yml up --scale worker=8 -d
```

### 3. Run the Search
```bash
docker compose -f docker/docker-compose.yml exec master dist_search \
  --master \
  --file data.txt \
  --token HELLO \
  --algo kmp \
  --workers docker-worker-1,docker-worker-2,docker-worker-3,docker-worker-4,docker-worker-5,docker-worker-6,docker-worker-7,docker-worker-8
```
### 4. Run Full Benchmarking Suite
This script will build the images, run all experiments, generate a CSV of results, and create plots. If you decide to run the full suite there will be no need to do the previous steps, but keep in mind that the full suite takes a relatively long time to finish.

```bash
./run_experiments.sh
```

## Message Passing Interface (MPI)

While this project does not use a traditional MPI library (like OpenMPI), it implements a custom **Message Passing Interface** using POSIX TCP Sockets to facilitate distributed communication. 

### Communication Protocol
- **Initialization Phase:** The Master establishes a persistent TCP connection with each Worker and sends the search algorithm and target token.
- **Task Streaming:** The Master streams file chunks to available Workers. Each message consists of a header (data length) followed by the raw chunk data.
- **Result Reporting:** Workers process the data and send back a structured result containing match offsets, newline counts, and boundary information.
- **Synchronization:** The system uses a bounded Producer-Consumer queue to synchronize the file-reading thread with multiple worker-manager threads, ensuring optimal throughput and preventing memory overflow.

This socket-based approach allows for flexible deployment across diverse containerized environments where a full MPI stack might not be pre-installed.