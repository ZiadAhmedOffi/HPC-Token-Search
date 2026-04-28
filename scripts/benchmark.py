"""
Author: Mahmoud Atef
Purpose: Robust benchmarking script with reliable timing and error reporting.
"""
import subprocess
import time
import csv
import os
import platform

def generate_data(size_mb, filename):
    print(f"Generating {size_mb}MB of data in {filename}...")
    with open(filename, 'w') as f:
        pattern = "HELLO"
        content = "ABCDEFGHIJKLMNOPQRSTUVWXYZ" * 1000 + pattern
        # Write in larger blocks
        block = content * 100
        repeats = (size_mb * 1024 * 1024) // len(block)
        for _ in range(repeats):
            f.write(block)
        # Handle remainder
        remainder = (size_mb * 1024 * 1024) % len(block)
        f.write(block[:remainder])

def run_experiment(algo, num_workers, file_path, token, chunk_size):
    # Ensure clean state before starting
    subprocess.run(["docker", "compose", "-f", "docker/docker-compose.yml", "down"], capture_output=True)
    
    # Scale workers
    subprocess.run(["docker", "compose", "-f", "docker/docker-compose.yml", "up", "--scale", f"worker={num_workers}", "-d"], check=True)
    
    # Wait for workers to be ready
    time.sleep(20)
    
    # Get worker container names - filter by project to avoid naming conflicts
    result = subprocess.run(["docker", "ps", "--format", "{{.Names}}", "--filter", "label=com.docker.compose.service=worker"], capture_output=True, text=True)
    all_names = result.stdout.strip().split("\n")
    
    # Clean and filter names
    worker_names = [name.strip() for name in all_names if name.strip()]
    worker_names.sort() # Ensure deterministic order
    
    worker_list = ",".join(worker_names[:num_workers])
    
    if not worker_list:
        print("Error: No workers found!")
        subprocess.run(["docker", "compose", "-f", "docker/docker-compose.yml", "down"], capture_output=True)
        return -1.0

    cmd = [
        "docker", "compose", "-f", "docker/docker-compose.yml", "exec", "-T", "master",
        "dist_search", "--master", "--file", file_path, "--token", token,
        "--algo", algo, "--workers", worker_list, "--chunk_size", str(chunk_size)
    ]
    
    print(f"Running: {algo} with {num_workers} workers (Workers: {worker_list})...")
    
    exec_time = -1.0
    for attempt in range(2): # Try up to 2 times
        start_wall = time.time()
        res = subprocess.run(cmd, capture_output=True, text=True)
        end_wall = time.time()
        
        if "Found" in res.stdout:
            # Try to parse time from C++ output
            for line in res.stdout.split("\n"):
                if "Execution time:" in line:
                    try:
                        exec_time = float(line.split(":")[1].split()[0])
                        break
                    except:
                        pass
            if exec_time <= 0:
                exec_time = end_wall - start_wall
            print(f"Success! Time: {exec_time}s")
            break # Success
        else:
            print(f"Attempt {attempt+1} failed for {algo} with {num_workers} workers.")
            print(f"STDOUT: {res.stdout}")
            print(f"STDERR: {res.stderr}")
            if attempt == 0:
                time.sleep(5) # Wait before retry
            
    subprocess.run(["docker", "compose", "-f", "docker/docker-compose.yml", "down"], capture_output=True)
    return exec_time

def main():
    algos = ["naive", "kmp", "rabin-karp"]
    worker_counts = [1, 2, 4, 8]
    filename = "data.txt_5GB" 
    token = "HELLO"
    chunk_size = 64
    runs = 3
    
    if not os.path.exists(filename):
        print(f"Data file {filename} not found. Generating...")
        generate_data(10000, filename)

    print("Building Docker images...")
    subprocess.run(["docker", "compose", "-f", "docker/docker-compose.yml", "build"], check=True)
    
    results = []
    for algo in algos:
        for workers in worker_counts:
            for run_id in range(runs):
                print(f"--- Algo: {algo}, Workers: {workers}, Run: {run_id+1}/{runs} ---")
                t = run_experiment(algo, workers, filename, token, chunk_size)
                results.append({
                    "algorithm": algo,
                    "workers": workers,
                    "run_id": run_id + 1,
                    "execution_time": t
                })

    with open("results.csv", "w", newline="") as f:
        if results:
            writer = csv.DictWriter(f, fieldnames=results[0].keys())
            writer.writeheader()
            writer.writerows(results)
    
    print("Results saved to results.csv")

if __name__ == "__main__":
    main()
