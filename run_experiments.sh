#!/bin/bash
# Author: Ziad Ahmed
# Purpose: Automation script to build the project, run benchmarks, and generate plots.

# Activate the correct venv
source ../HPC_Project/hpc_venv/bin/activate

# Run benchmarks (this now handles docker build)
python3 scripts/benchmark.py

# Generate plots
python3 scripts/visualize.py

echo "Experiments finished. Results in results.csv and report/figures/"
