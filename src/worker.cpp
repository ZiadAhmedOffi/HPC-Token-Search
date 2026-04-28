/**
 * Author: Mahmoud Atef
 * Purpose: Worker implementation for processing search tasks.
 * 
 * The worker listens for incoming connections from the Master, receives 
 * search parameters (algorithm and pattern), and then processes text 
 * chunks sequentially.
 */
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include "algorithms.hpp"
#include "socket_utils.hpp"
#include "common.hpp"

/**
 * Processes a single text chunk using the specified algorithm.
 * Calculates local line numbers and columns for each match.
 * 
 * Implementation Details:
 * - Line/Col Calculation: Uses memchr to efficiently find newline characters.
 * - Result Metadata: Tracks total newlines and last line length for Master-side aggregation.
 */
SearchResult process_task(const std::string& algo_name, const std::string& pattern, const std::string& chunk) {
    SearchResult res;
    res.total_newlines = 0;
    res.last_line_len = 0;

    SearchFunc search = get_algorithm(algo_name);
    if (!search) return res;

    // Perform the string search
    std::vector<long> offsets = search(chunk, pattern);
    
    // Efficiently count newlines and find the position of the last newline
    int last_newline_pos = -1;
    const char* data = chunk.c_str();
    size_t len = chunk.length();
    size_t pos = 0;
    while (pos < len) {
        const char* found = (const char*)memchr(data + pos, '\n', len - pos);
        if (!found) break;
        res.total_newlines++;
        last_newline_pos = found - data;
        pos = last_newline_pos + 1;
    }
    // Calculate length of the trailing text after the last newline
    res.last_line_len = len - (last_newline_pos + 1);

    if (offsets.empty()) return res;

    // Create a lookup table of newline positions for fast line/col calculation
    std::vector<long> newline_pos;
    newline_pos.push_back(-1); // Virtual newline before the start of the chunk
    pos = 0;
    while (pos < len) {
        const char* found = (const char*)memchr(data + pos, '\n', len - pos);
        if (!found) break;
        long current_pos = found - data;
        newline_pos.push_back(current_pos);
        pos = current_pos + 1;
    }

    // Convert byte offsets to 1-based line numbers and 0-based columns
    for (long offset : offsets) {
        auto it = std::lower_bound(newline_pos.begin(), newline_pos.end(), offset);
        it--; // Points to the newline immediately preceding the match
        int line_idx = std::distance(newline_pos.begin(), it);
        int col = offset - *it - (it == newline_pos.begin() ? 1 : 0);
        res.matches.push_back({offset, line_idx + 1, col});
    }

    return res;
}

/**
 * Main worker loop.
 * 
 * Fixed Issue: Resetting 'current_addrlen' before each accept() to ensure 
 * consistent behavior across different Linux distributions.
 */
void run_worker(int port) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;

    // Create and configure the server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) exit(EXIT_FAILURE);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) exit(EXIT_FAILURE);
    if (listen(server_fd, 10) < 0) exit(EXIT_FAILURE);

    std::cout << "[Worker] Listening on port " << port << "..." << std::endl;

    while (true) {
        socklen_t current_addrlen = sizeof(address);
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &current_addrlen)) < 0) continue;

        // Receive algo and pattern once per connection
        std::string algo_name, pattern;
        if (!receive_string(new_socket, algo_name) || !receive_string(new_socket, pattern)) {
            close(new_socket);
            continue;
        }

        // Keep connection open for multiple tasks (chunk-based streaming)
        while (true) {
            std::string chunk;
            if (!receive_string(new_socket, chunk)) break;

            SearchResult res = process_task(algo_name, pattern, chunk);
            if (!send_search_result(new_socket, res)) break;
        }
        close(new_socket);
    }
}
