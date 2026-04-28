/**
 * Author: Mahmoud Atef
 * Purpose: Entry point for the distributed search application.
 * 
 * This file handles command-line argument parsing and dispatches execution to 
 * either the Master or Worker logic.
 */
#include <iostream>
#include <vector>
#include <string>
#include <getopt.h>
#include "common.hpp"

// Function prototypes from other files
void run_master(const std::string& filename, const std::string& pattern, const std::string& algo, const std::vector<std::string>& workers, size_t chunk_size_bytes);
void run_worker(int port);

/**
 * Displays usage instructions for both Master and Worker modes.
 */
void print_usage() {
    std::cout << "Usage (Master): dist_search --master --file <file> --token <token> --algo <naive|kmp|rabin-karp> --workers <host1,host2,...> [--chunk_size <MB>]" << std::endl;
    std::cout << "Usage (Worker): dist_search --worker [--port <port>]" << std::endl;
}

int main(int argc, char* argv[]) {
    bool is_master = false;
    bool is_worker = false;
    std::string filename;
    std::string token;
    std::string algo = "kmp";
    std::vector<std::string> workers;
    int port = DEFAULT_PORT;
    size_t chunk_size_mb = CHUNK_SIZE_MB;

    // Define supported command line options
    static struct option long_options[] = {
        {"master", no_argument, 0, 'm'},
        {"worker", no_argument, 0, 'w'},
        {"file", required_argument, 0, 'f'},
        {"token", required_argument, 0, 't'},
        {"algo", required_argument, 0, 'a'},
        {"workers", required_argument, 0, 'L'},
        {"port", required_argument, 0, 'p'},
        {"chunk_size", required_argument, 0, 'c'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int c;
    while ((c = getopt_long(argc, argv, "mwf:t:a:L:p:c:", long_options, &option_index)) != -1) {
        switch (c) {
            case 'm': is_master = true; break;
            case 'w': is_worker = true; break;
            case 'f': filename = optarg; break;
            case 't': token = optarg; break;
            case 'a': algo = optarg; break;
            case 'L': {
                // Split comma-separated list of workers
                std::string s = optarg;
                size_t pos = 0;
                while ((pos = s.find(',')) != std::string::npos) {
                    std::string w = s.substr(0, pos);
                    if (!w.empty()) workers.push_back(w);
                    s.erase(0, pos + 1);
                }
                if (!s.empty()) workers.push_back(s);
                break;
            }
            case 'p': port = std::stoi(optarg); break;
            case 'c': chunk_size_mb = std::stoul(optarg); break;
            default: print_usage(); return 1;
        }
    }

    // Mutually exclusive role check
    if (is_master && is_worker) {
        std::cerr << "Error: Process cannot be both master and worker simultaneously." << std::endl;
        return 1;
    }

    // Role-based dispatch
    if (is_master) {
        if (filename.empty() || token.empty() || workers.empty()) {
            std::cerr << "Error: Master mode requires --file, --token, and --workers." << std::endl;
            print_usage();
            return 1;
        }
        run_master(filename, token, algo, workers, chunk_size_mb * 1024 * 1024);
    } else if (is_worker) {
        run_worker(port);
    } else {
        std::cerr << "Error: You must specify either --master or --worker." << std::endl;
        print_usage();
        return 1;
    }

    return 0;
}
