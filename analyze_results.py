"""
Parallel Sobel Edge Detection — Performance Analysis
EE7218 / EC7207: High Performance Computing

Run after benchmark.sh completes:
    python3 analyze_results.py

Outputs:
  - Console: formatted speedup/efficiency tables
  - results/speedup_table.csv
  - results/performance_summary.csv
"""

import csv
import math
import os
from collections import defaultdict

RESULTS_DIR = "results"
TIMING_CSV  = os.path.join(RESULTS_DIR, "benchmark_results.csv")
RMSE_CSV    = os.path.join(RESULTS_DIR, "rmse_results.csv")


# ─── Load timing data ────────────────────────────────────────────────────────

def load_timing(path):
    """Returns dict: (impl, img_size, threads) -> [time_ms, ...]"""
    data = defaultdict(list)
    with open(path) as f:
        reader = csv.DictReader(f)
        for row in reader:
            key = (row["implementation"], row["image_size"], row["threads_procs"])
            data[key].append(float(row["time_ms"]))
    return data


def average(times):
    return sum(times) / len(times) if times else None


# ─── Load RMSE data ──────────────────────────────────────────────────────────

def load_rmse(path):
    """Returns dict: (impl, img_size, threads) -> (rmse, psnr)"""
    data = {}
    if not os.path.exists(path):
        return data
    with open(path) as f:
        reader = csv.DictReader(f)
        for row in reader:
            key = (row["implementation"], row["image_size"], row["threads_procs"])
            rmse = row.get("rmse", "")
            psnr = row.get("psnr_db", "")
            if rmse:
                data[key] = (float(rmse), float(psnr) if psnr else float("inf"))
    return data


# ─── Format helpers ──────────────────────────────────────────────────────────

def fmt(val, decimals=2):
    if val is None:
        return "N/A"
    return f"{val:.{decimals}f}"


def print_table(headers, rows, col_widths=None):
    if col_widths is None:
        col_widths = [max(len(str(r[i])) for r in ([headers] + rows)) + 2
                      for i in range(len(headers))]
    fmt_row = lambda r: " | ".join(str(c).ljust(w) for c, w in zip(r, col_widths))
    sep = "-+-".join("-" * w for w in col_widths)
    print(fmt_row(headers))
    print(sep)
    for row in rows:
        print(fmt_row(row))
    print()


# ─── Main analysis ───────────────────────────────────────────────────────────

def main():
    if not os.path.exists(TIMING_CSV):
        print(f"Error: {TIMING_CSV} not found. Run benchmark.sh first.")
        return

    timing = load_timing(TIMING_CSV)
    rmse_data = load_rmse(RMSE_CSV)

    # Thread/process configurations to compare
    shared_memory_configs = ["1", "2", "4", "8"]
    mpi_configs           = ["1", "2", "4", "8"]
    hybrid_configs        = ["2proc_2thr", "2proc_4thr", "4proc_2thr", "1proc_8thr"]

    image_sizes = ["512x512", "4K"]
    impls_shared = ["openmp", "pthreads"]
    impls_dist   = ["mpi"]
    impls_hybrid = ["hybrid"]

    speedup_rows = []  # for CSV export
    perf_rows    = []  # full summary for CSV

    for img in image_sizes:
        print("=" * 72)
        print(f"  IMAGE SIZE: {img}")
        print("=" * 72)

        # Serial baseline
        serial_key = ("serial", img, "1")
        serial_times = timing.get(serial_key, [])
        serial_avg   = average(serial_times)

        if serial_avg is None:
            print(f"  [WARN] No serial result for {img}. Skipping.\n")
            continue

        print(f"\nSerial baseline: {fmt(serial_avg)} ms\n")

        # ── Shared-memory table (OpenMP + Pthreads) ──────────────────────────
        print("--- Shared-Memory Parallelism (OpenMP / Pthreads) ---")
        headers = ["Impl", "Threads", "Avg Time (ms)", "Speedup", "Efficiency (%)",
                   "RMSE", "PSNR (dB)"]
        rows = []
        for impl in impls_shared:
            for T in shared_memory_configs:
                key = (impl, img, T)
                times = timing.get(key, [])
                avg   = average(times)
                if avg is None:
                    continue
                speedup    = serial_avg / avg
                efficiency = (speedup / int(T)) * 100

                rmse_key = (impl, img, T)
                r, p = rmse_data.get(rmse_key, (None, None))
                psnr_str = "inf" if p == float("inf") else fmt(p, 2)

                rows.append([impl, T, fmt(avg), fmt(speedup), fmt(efficiency),
                              fmt(r, 4) if r is not None else "N/A", psnr_str])
                speedup_rows.append([impl, img, T, fmt(avg), fmt(speedup), fmt(efficiency)])
                perf_rows.append([impl, img, T, fmt(avg), fmt(speedup), fmt(efficiency),
                                  fmt(r, 4) if r is not None else "N/A", psnr_str])

        if rows:
            print_table(headers, rows)

        # ── MPI table ────────────────────────────────────────────────────────
        print("--- Distributed-Memory Parallelism (MPI) ---")
        rows = []
        for NP in mpi_configs:
            key = ("mpi", img, NP)
            times = timing.get(key, [])
            avg   = average(times)
            if avg is None:
                continue
            speedup    = serial_avg / avg
            efficiency = (speedup / int(NP)) * 100

            rmse_key = ("mpi", img, NP)
            r, p = rmse_data.get(rmse_key, (None, None))
            psnr_str = "inf" if p == float("inf") else fmt(p, 2)

            rows.append(["mpi", NP, fmt(avg), fmt(speedup), fmt(efficiency),
                          fmt(r, 4) if r is not None else "N/A", psnr_str])
            speedup_rows.append(["mpi", img, NP, fmt(avg), fmt(speedup), fmt(efficiency)])
            perf_rows.append(["mpi", img, NP, fmt(avg), fmt(speedup), fmt(efficiency),
                              fmt(r, 4) if r is not None else "N/A", psnr_str])

        if rows:
            print_table(headers, rows)

        # ── Hybrid table ─────────────────────────────────────────────────────
        print("--- Hybrid (MPI + OpenMP) ---")
        headers_h = ["Config (P×T)", "Parallel Units", "Avg Time (ms)",
                     "Speedup", "Efficiency (%)", "RMSE", "PSNR (dB)"]
        rows = []
        for cfg in hybrid_configs:
            parts = cfg.replace("proc_", "x").replace("thr", "").split("x")
            P, T  = int(parts[0]), int(parts[1])
            units = P * T
            key   = ("hybrid", img, cfg)
            times = timing.get(key, [])
            avg   = average(times)
            if avg is None:
                continue
            speedup    = serial_avg / avg
            efficiency = (speedup / units) * 100

            rmse_key = ("hybrid", img, cfg)
            r, p = rmse_data.get(rmse_key, (None, None))
            psnr_str = "inf" if p == float("inf") else fmt(p, 2)

            rows.append([f"{P}×{T}", str(units), fmt(avg), fmt(speedup),
                          fmt(efficiency), fmt(r, 4) if r is not None else "N/A", psnr_str])
            speedup_rows.append(["hybrid", img, cfg, fmt(avg), fmt(speedup), fmt(efficiency)])
            perf_rows.append(["hybrid", img, cfg, fmt(avg), fmt(speedup), fmt(efficiency),
                              fmt(r, 4) if r is not None else "N/A", psnr_str])

        if rows:
            print_table(headers_h, rows)

        # ── Amdahl's Law estimate ─────────────────────────────────────────────
        # Use best speedup observed to back-calculate parallel fraction
        all_speedups = []
        for impl in ["openmp", "pthreads", "mpi"]:
            for T in ["2", "4", "8"]:
                key = (impl, img, T)
                times = timing.get(key, [])
                avg = average(times)
                if avg:
                    all_speedups.append((serial_avg / avg, int(T)))

        if all_speedups:
            # Best speedup at N=8 for Amdahl estimate
            best = max((s for s, n in all_speedups if n == 8), default=None)
            if best and best > 1:
                # S = 1 / (1-p + p/N)  => p = (1/S - 1) / (1/N - 1)
                N = 8
                p = (1.0 / best - 1.0) / (1.0 / N - 1.0)
                p = max(0.0, min(1.0, p))
                print(f"  Amdahl's Law estimate (from best 8-unit speedup = {fmt(best)}x):")
                print(f"    Parallel fraction p ≈ {p:.3f} ({p*100:.1f}%)")
                print(f"    Theoretical max speedup = {1/(1-p):.2f}x")
                print()

    # ── Write summary CSVs ────────────────────────────────────────────────────
    os.makedirs(RESULTS_DIR, exist_ok=True)

    with open(os.path.join(RESULTS_DIR, "performance_summary.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["implementation", "image_size", "threads_procs",
                    "avg_time_ms", "speedup", "efficiency_pct", "rmse", "psnr_db"])
        w.writerows(perf_rows)

    print(f"Summary saved to {RESULTS_DIR}/performance_summary.csv")

    # Print RMSE accuracy summary
    print()
    print("=" * 72)
    print("  ACCURACY SUMMARY (RMSE vs Serial Reference)")
    print("=" * 72)
    print(f"{'Impl':<12} {'Image':<8} {'Config':<14} {'RMSE':>10} {'PSNR (dB)':>12}")
    print("-" * 60)
    for (impl, img, cfg), (r, p) in sorted(rmse_data.items()):
        psnr_str = "inf (perfect)" if p == float("inf") else f"{p:.2f}"
        print(f"{impl:<12} {img:<8} {cfg:<14} {r:>10.6f} {psnr_str:>12}")


if __name__ == "__main__":
    main()
