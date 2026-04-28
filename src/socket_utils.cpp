/**
 * Author: Yassin Ahmed
 * Purpose: Robust implementation of socket utilities with error handling and defensive checks.
 * 
 * Implementation Notes:
 * TCP is a stream protocol, meaning messages can be fragmented or coalesced. 
 * To ensure reliability, we use `send_all` and `recv_all` to guarantee that the 
 * requested number of bytes is fully processed before returning.
 */
#include "socket_utils.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <string>
#include "common.hpp"

/**
 * Ensures all requested bytes are sent, handling partial writes.
 * Fixed Issue: Prevents "incomplete data" errors when sending large text chunks.
 */
static bool send_all(int socket, const void* buffer, size_t length) {
    const char* ptr = static_cast<const char*>(buffer);
    while (length > 0) {
        ssize_t sent = send(socket, ptr, length, 0);
        if (sent <= 0) return false;
        ptr += sent;
        length -= sent;
    }
    return true;
}

/**
 * Ensures all requested bytes are received, handling partial reads.
 * Fixed Issue: Prevents "missing header" or "corrupt payload" issues.
 */
static bool recv_all(int socket, void* buffer, size_t length) {
    char* ptr = static_cast<char*>(buffer);
    while (length > 0) {
        ssize_t received = recv(socket, ptr, length, MSG_WAITALL);
        if (received <= 0) return false;
        ptr += received;
        length -= received;
    }
    return true;
}

bool send_string(int socket, const std::string& s) {
    long len = s.length();
    if (!send_all(socket, &len, sizeof(len))) return false;
    if (len > 0) {
        if (!send_all(socket, s.c_str(), len)) return false;
    }
    return true;
}

bool receive_string(int socket, std::string& s) {
    long len;
    if (!recv_all(socket, &len, sizeof(len))) return false;
    // Defensive check: limit max string size to 512MB to prevent OOM
    if (len < 0 || len > 1024 * 1024 * 512) return false;
    if (len == 0) { s = ""; return true; }
    s.resize(len);
    if (!recv_all(socket, &s[0], len)) return false;
    return true;
}

bool send_long(int socket, long val) {
    return send_all(socket, &val, sizeof(val));
}

bool receive_long(int socket, long& val) {
    return recv_all(socket, &val, sizeof(val));
}

/**
 * Serializes SearchResult for network transfer.
 * Note: Uses binary copy for the Match vector for speed, assuming identical 
 * architecture (Endianness) between Master and Worker.
 */
bool send_search_result(int socket, const SearchResult& res) {
    long count = res.matches.size();
    if (!send_all(socket, &count, sizeof(count))) return false;
    if (count > 0) {
        if (!send_all(socket, res.matches.data(), count * sizeof(Match))) return false;
    }
    if (!send_all(socket, &res.total_newlines, sizeof(int))) return false;
    if (!send_all(socket, &res.last_line_len, sizeof(int))) return false;
    return true;
}

/**
 * Deserializes SearchResult with safety checks.
 * Fixed Issue: Prevents crashes if a corrupted worker returns an impossible number of matches.
 */
bool receive_search_result(int socket, SearchResult& res) {
    long count;
    if (!recv_all(socket, &count, sizeof(count))) return false;
    // Defensive check: limit max matches to 100 million to prevent allocation failure
    if (count < 0 || count > 100000000) return false;
    res.matches.resize(count);
    if (count > 0) {
        if (!recv_all(socket, res.matches.data(), count * sizeof(Match))) return false;
    }
    if (!recv_all(socket, &res.total_newlines, sizeof(int))) return false;
    if (!recv_all(socket, &res.last_line_len, sizeof(int))) return false;
    return true;
}
