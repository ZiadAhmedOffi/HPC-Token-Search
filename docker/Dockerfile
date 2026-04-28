# Author: Ziad Ahmed
# Purpose: Multi-stage Dockerfile for reliable C++ builds on Alpine.

# Stage 1: Build
FROM alpine:latest AS builder
RUN apk add --no-cache g++ make
WORKDIR /build
COPY . .
RUN make clean && make

# Stage 2: Runtime
FROM alpine:latest
RUN apk add --no-cache libstdc++
WORKDIR /app
COPY --from=builder /build/dist_search /usr/local/bin/dist_search
# Data will be provided via volume in docker-compose for flexibility
ENTRYPOINT ["dist_search"]
