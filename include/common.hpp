/**
 * Author: Mahmoud Atef
 * Purpose: Shared structures and constants for the distributed search application.
 * 
 * This file defines the core data exchange formats used between the Master and Workers.
 * Accurate tracking of line and column data is critical for reconstructing global positions
 * from local chunk searches.
 */
#ifndef COMMON_HPP
#define COMMON_HPP

#include <string>
#include <vector>

/**
 * Structure representing a single pattern match within a text.
 */
struct Match {
    long offset; // The byte offset where the match starts (global or relative to chunk)
    int line;    // The 1-based line number within the chunk/file
    int column;  // The 0-based column index within that line
};

/**
 * The consolidated result returned by a Worker to the Master after processing a chunk.
 * 
 * Solving Aggregation Issues:
 * To correctly calculate the global line number for a match in chunk N, the Master must 
 * know the total number of newlines in all previous chunks (0 to N-1). Similarly, the 
 * column of the first line in chunk N depends on the length of the last line in chunk N-1.
 * These fields ensure the Master has the necessary metadata to stitch results together.
 */
struct SearchResult {
    std::vector<Match> matches; // List of all matches found in the specific chunk
    int total_newlines;         // Total count of '\n' characters found in this chunk
    int last_line_len;          // The number of characters after the last '\n' in this chunk
};

#define DEFAULT_PORT 8080       // Default port for worker listening
#define CHUNK_SIZE_MB 2         // Default chunk size for file distribution

#endif
