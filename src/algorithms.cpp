/**
 * Author: Ahmed Osama
 * Purpose: Implementation of string matching algorithms (Naive, KMP, Rabin-Karp).
 * 
 * Each algorithm returns a list of byte offsets where the pattern starts.
 */
#include "algorithms.hpp"
#include <iostream>
#include <vector>
#include <string>

/**
 * Naive search implementation.
 * Performs a simple brute-force comparison.
 */
std::vector<long> naive_search(const std::string& text, const std::string& pattern) {
    std::vector<long> matches;
    if (pattern.empty()) return matches;
    if (text.length() < pattern.length()) return matches;

    for (size_t i = 0; i <= text.length() - pattern.length(); ++i) {
        bool found = true;
        for (size_t j = 0; j < pattern.length(); ++j) {
            if (text[i + j] != pattern[j]) {
                found = false;
                break;
            }
        }
        if (found) {
            matches.push_back((long)i);
        }
    }
    return matches;
}

/**
 * Knuth-Morris-Pratt (KMP) algorithm.
 * Uses a Longest Prefix Suffix (LPS) table to skip redundant comparisons.
 * Complexity: O(N+M)
 */
std::vector<long> kmp_search(const std::string& text, const std::string& pattern) {
    std::vector<long> matches;
    if (pattern.empty()) return matches;
    
    int m = pattern.length();
    int n = text.length();
    if (n < m) return matches;

    // Preprocess pattern to create LPS table
    std::vector<int> lps(m, 0);
    for (int i = 1, len = 0; i < m; ) {
        if (pattern[i] == pattern[len]) {
            lps[i++] = ++len;
        } else if (len != 0) {
            len = lps[len - 1];
        } else {
            lps[i++] = 0;
        }
    }

    // Perform search using the LPS table
    int i = 0; // index for text
    int j = 0; // index for pattern
    while (i < n) {
        if (pattern[j] == text[i]) {
            i++;
            j++;
        }
        if (j == m) {
            matches.push_back((long)(i - j));
            j = lps[j - 1];
        } else if (i < n && pattern[j] != text[i]) {
            if (j != 0) {
                j = lps[j - 1];
            } else {
                i++;
            }
        }
    }
    return matches;
}

/**
 * Rabin-Karp algorithm.
 * Uses a rolling hash function to identify potential match locations.
 * Complexity: O(N+M) average, O(N*M) worst case.
 */
std::vector<long> rabin_karp_search(const std::string& text, const std::string& pattern) {
    std::vector<long> matches;
    if (pattern.empty()) return matches;

    int n = text.length();
    int m = pattern.length();
    if (n < m) return matches;

    const long long d = 256;         // Alphabet size
    const long long q = 1000000007;  // A large prime for hashing
    long long h = 1;                 // The value of d^(m-1) % q
    long long p = 0;                 // Hash value for pattern
    long long t = 0;                 // Hash value for current window in text

    // Precalculate h
    for (int i = 0; i < m - 1; i++)
        h = (h * d) % q;

    // Calculate initial hashes
    for (int i = 0; i < m; i++) {
        p = (d * p + (unsigned char)pattern[i]) % q;
        t = (d * t + (unsigned char)text[i]) % q;
    }

    for (int i = 0; i <= n - m; i++) {
        // If hashes match, perform a precise comparison to handle collisions
        if (p == t) {
            bool found = true;
            for (int j = 0; j < m; j++) {
                if (text[i + j] != pattern[j]) {
                    found = false;
                    break;
                }
            }
            if (found) matches.push_back((long)i);
        }
        
        // Slide the window and update the hash
        if (i < n - m) {
            t = (d * (t - (unsigned char)text[i] * h) + (unsigned char)text[i + m]) % q;
            if (t < 0) t = (t + q);
        }
    }
    return matches;
}

SearchFunc get_algorithm(const std::string& name) {
    if (name == "naive") return naive_search;
    if (name == "kmp") return kmp_search;
    if (name == "rabin-karp") return rabin_karp_search;
    return nullptr;
}
