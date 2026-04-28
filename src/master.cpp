/**
 * Author: Ziad Ahmed
 * Purpose: Parallel Distributed Master implementation with multi-threaded task dispatching.
 * 
 * The Master now utilizes a Producer-Consumer model:
 * 1. Main Thread (Producer): Reads the input file in chunks and pushes them into a thread-safe queue.
 * 2. Worker Threads (Consumers): Each thread manages one worker connection, pulls tasks from the queue, 
 *    sends them to its assigned worker, and aggregates results.
 * 
 * Correctness: Handles matches spanning across chunks using an overlap buffer and filtering logic.
 * Performance: Achieves true concurrency by keeping all workers busy simultaneously.
 */
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <mutex>
#include <map>
#include <queue>
#include <condition_variable>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <cstring>
#include <utility>
#include "common.hpp"
#include "socket_utils.hpp"

// Task structure representing a unit of work
struct Task {
    int chunk_id;
    std::string data;
    size_t overlap_len;
};

// Global synchronization and state
std::map<int, SearchResult> final_results;
std::mutex results_mutex;
std::queue<Task> task_queue;
std::mutex queue_mutex;
std::condition_variable queue_cv;    // For consumers waiting for tasks
std::condition_variable full_cv;     // For producer waiting when queue is full
bool finished_reading = false;

/**
 * Worker thread function. Each thread manages communication with one remote worker node.
 */
void worker_thread_func(int sock, std::string host, std::string pattern) {
    long pattern_len = (long)pattern.length();
    
    while (true) {
        Task task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            queue_cv.wait(lock, [] { return !task_queue.empty() || finished_reading; });
            
            if (task_queue.empty() && finished_reading) break;
            
            task = std::move(task_queue.front());
            task_queue.pop();
        }
        full_cv.notify_one();

        // Send task to worker and receive result
        if (send_string(sock, task.data)) {
            SearchResult sr;
            if (receive_search_result(sock, sr)) {
                // Filter matches to avoid double counting across chunk boundaries.
                // A match is "new" if it ends outside the overlap prepended by the master.
                std::vector<Match> filtered;
                for (auto& m : sr.matches) {
                    if (task.chunk_id == 0 || m.offset + pattern_len > (long)task.overlap_len) {
                        filtered.push_back(m);
                    }
                }
                sr.matches = std::move(filtered);

                std::lock_guard<std::mutex> lock(results_mutex);
                final_results[task.chunk_id] = std::move(sr);
            } else {
                std::cerr << "[Master] Worker " << host << " failed during receive. Re-queueing task " << task.chunk_id << std::endl;
                {
                    std::lock_guard<std::mutex> lock(queue_mutex);
                    task_queue.push(std::move(task));
                }
                queue_cv.notify_one();
                break; // Terminate this specific worker thread
            }
        } else {
            std::cerr << "[Master] Worker " << host << " failed during send. Re-queueing task " << task.chunk_id << std::endl;
            {
                std::lock_guard<std::mutex> lock(queue_mutex);
                task_queue.push(std::move(task));
            }
            queue_cv.notify_one();
            break; // Terminate this specific worker thread
        }
    }
    close(sock);
}

void run_master(const std::string& filename, const std::string& pattern, const std::string& algo, const std::vector<std::string>& workers, size_t chunk_size_bytes) {
    std::cout << "[Master] Initializing parallel search for pattern: \"" << pattern << "\" using " << algo << std::endl;
    
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "[Master] Error: Could not open file: " << filename << std::endl;
        return;
    }

    // Overlap length is needed to catch matches spanning across chunks
    size_t overlap_len = pattern.length() > 0 ? pattern.length() - 1 : 0;
    
    // Initialize/Reset shared state
    {
        std::lock_guard<std::mutex> lock(results_mutex);
        final_results.clear();
    }
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        while(!task_queue.empty()) task_queue.pop();
        finished_reading = false;
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // --- Connection Phase ---
    std::vector<std::thread> worker_threads;
    std::cout << "[Master] Connecting to workers..." << std::endl;
    
    for (const std::string& host : workers) {
        if (host.empty()) continue;

        struct addrinfo hints, *res = nullptr;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        
        if (getaddrinfo(host.c_str(), "8080", &hints, &res) != 0 || !res) continue;

        int sock = -1;
        for (int i = 0; i < 5; ++i) {
            sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
            if (sock < 0) break;
            if (connect(sock, res->ai_addr, res->ai_addrlen) >= 0) break;
            close(sock);
            sock = -1;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        freeaddrinfo(res);

        if (sock >= 0) {
            if (send_string(sock, algo) && send_string(sock, pattern)) {
                std::cout << "[Master] Connected to " << host << std::endl;
                worker_threads.emplace_back(worker_thread_func, sock, host, pattern);
            } else {
                close(sock);
            }
        }
    }

    if (worker_threads.empty()) {
        std::cerr << "[Master] Error: No workers available. Aborting." << std::endl;
        return;
    }

    // --- Distribution Phase ---
    int chunk_id = 0;
    std::string overlap_data = "";
    std::vector<char> buf(chunk_size_bytes);

    while (true) {
        file.read(buf.data(), chunk_size_bytes);
        size_t n = file.gcount();
        if (n == 0) break;
        
        // Construct task data by prepending overlap from the previous chunk
        std::string task_chunk = overlap_data + std::string(buf.data(), n);
        size_t current_overlap_len = overlap_data.length();

        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            // Bounded queue to balance memory usage and worker throughput
            full_cv.wait(lock, [] { return task_queue.size() < 32; });
            task_queue.push({chunk_id++, std::move(task_chunk), current_overlap_len});
        }
        queue_cv.notify_one();

        // Update overlap buffer for the next chunk
        if (n >= overlap_len) {
            overlap_data = std::string(buf.data() + n - overlap_len, overlap_len);
        } else {
            std::string temp = overlap_data + std::string(buf.data(), n);
            overlap_data = (temp.length() >= overlap_len) ? temp.substr(temp.length() - overlap_len) : temp;
        }
    }

    // Signal workers that reading is complete
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        finished_reading = true;
    }
    queue_cv.notify_all();

    // Wait for all worker threads to finish processing
    for (auto& t : worker_threads) {
        if (t.joinable()) t.join();
    }

    // --- Final Results Aggregation ---
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end_time - start_time;

    long total_matches = 0;
    // Map remains sorted by chunk_id, ensuring deterministic aggregation
    for (auto const& it : final_results) {
        total_matches += it.second.matches.size();
    }

    std::cout << "Found " << total_matches << " matches." << std::endl;
    std::cout << "Execution time: " << diff.count() << " seconds" << std::endl << std::flush;
}
