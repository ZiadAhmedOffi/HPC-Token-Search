/**
 * Author: Ahmed Osama 
 * Purpose: Interface definitions for distributed string matching algorithms.
 */
#ifndef ALGORITHMS_HPP
#define ALGORITHMS_HPP

#include <string>
#include <vector>
#include "common.hpp"

/**
 * Signature for string search functions.
 * @param text The text chunk to search within.
 * @param pattern The pattern to look for.
 * @return A vector of byte offsets where the pattern was found.
 */
typedef std::vector<long> (*SearchFunc)(const std::string&, const std::string&);

/**
 * Simple O(N*M) search. Reliable but slow for large patterns.
 */
std::vector<long> naive_search(const std::string& text, const std::string& pattern);

/**
 * Knuth-Morris-Pratt algorithm. Optimized O(N+M) search using a failure function.
 */
std::vector<long> kmp_search(const std::string& text, const std::string& pattern);

/**
 * Rabin-Karp algorithm. Uses rolling hashes to find matches in average O(N+M).
 */
std::vector<long> rabin_karp_search(const std::string& text, const std::string& pattern);

/**
 * Factory-style function to retrieve an algorithm implementation by its string name.
 * Used by the Worker to dynamically switch logic based on Master instructions.
 */
SearchFunc get_algorithm(const std::string& name);

#endif
