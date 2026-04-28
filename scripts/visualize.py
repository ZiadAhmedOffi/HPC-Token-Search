"""
Author: Yassin Ahmad
Purpose: Visualization script with improved error handling and statistics.
"""
import os
import csv
import pandas as pd
import matplotlib.pyplot as plt

def main():
    if not os.path.exists("results.csv") or os.stat("results.csv").st_size == 0:
        print("results.csv is empty or missing")
        return

    results = []
    with open("results.csv", "r") as f:
        reader = csv.DictReader(f)
        for row in reader:
            if float(row["execution_time"]) > 0:
                results.append({
                    "algorithm": row["algorithm"],
                    "workers": int(row["workers"]),
                    "run_id": int(row.get("run_id", 0)),
                    "execution_time": float(row["execution_time"])
                })

    if not results:
        print("No valid results found in results.csv")
        return

    
    df = pd.DataFrame(results)
    
    # Calculate statistics
    stats_df = df.groupby(["algorithm", "workers"])["execution_time"].agg(["mean", "std"]).reset_index()
    
    print("\n--- Summary Statistics ---")
    print(stats_df)

    os.makedirs("report/figures", exist_ok=True)

    # Plot 1: Execution Time vs Workers
    plt.figure(figsize=(10, 6))
    for algo in stats_df["algorithm"].unique():
        data = stats_df[stats_df["algorithm"] == algo]
        plt.errorbar(data["workers"], data["mean"], yerr=data["std"].fillna(0), label=algo, marker='o', capsize=5)
    
    plt.xlabel("Number of Workers")
    plt.ylabel("Mean Execution Time (s)")
    plt.title("Execution Time vs Workers (5GB File)")
    plt.legend()
    plt.grid(True)
    plt.savefig("report/figures/exec_time.png")
    print("Saved: report/figures/exec_time.png")
    plt.close()

    # Plot 2: Speedup vs Workers
    plt.figure(figsize=(10, 6))
    for algo in stats_df["algorithm"].unique():
        data = stats_df[stats_df["algorithm"] == algo]
        t1_row = data[data["workers"] == 1]
        if not t1_row.empty:
            t1 = t1_row["mean"].values[0]
            speedup = t1 / data["mean"]
            plt.plot(data["workers"], speedup, label=algo, marker='o')
    
    plt.plot([1, 8], [1, 8], 'k--', alpha=0.5, label="Ideal Speedup")
    plt.xlabel("Number of Workers")
    plt.ylabel("Speedup (T1 / Tn)")
    plt.title("Speedup vs Workers (5GB File)")
    plt.legend()
    plt.grid(True)
    plt.savefig("report/figures/speedup.png")
    print("Saved: report/figures/speedup.png")
    plt.close()

if __name__ == "__main__":
    main()
