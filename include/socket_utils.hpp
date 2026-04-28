/**
 * Author: Yassin Ahmed
 * Purpose: Robust socket utility declarations for Master-Worker communication.
 * 
 * These utilities abstract the low-level socket API to provide reliable transmission 
 * of complex data types (strings, results) over TCP.
 */
#ifndef SOCKET_UTILS_HPP
#define SOCKET_UTILS_HPP

#include <string>
#include <vector>
#include "common.hpp"

/**
 * Sends a string over a socket by first transmitting its length.
 * Handles partial sends and ensures the entire string is delivered.
 */
bool send_string(int socket, const std::string& s);

/**
 * Receives a string by first reading its length and then allocating the buffer.
 * Includes defensive checks to prevent memory exhaustion from malicious/corrupt length values.
 */
bool receive_string(int socket, std::string& s);

/**
 * Sends a long value (typically for sizes or offsets).
 */
bool send_long(int socket, long val);

/**
 * Receives a long value.
 */
bool receive_long(int socket, long& val);

/**
 * Serializes and sends a SearchResult structure.
 * Correctly handles the dynamic vector of matches.
 */
bool send_search_result(int socket, const SearchResult& res);

/**
 * Receives and deserializes a SearchResult structure.
 * Implements safety limits on the number of matches to avoid allocator crashes.
 */
bool receive_search_result(int socket, SearchResult& res);

#endif
