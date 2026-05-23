#!/usr/bin/env bash
# =============================================================================
#  Parallel Sobel Edge Detection — Performance Benchmark Script
#  EE7218 / EC7207: High Performance Computing
#
#  Usage:
#    chmod +x benchmark.sh
#    ./benchmark.sh
#
#  Output:
#    results/benchmark_results.csv  — raw timing data
#    results/rmse_results.csv       — accuracy (RMSE/PSNR) data
#
#  Prerequisites (macOS / Linux):
#    make serial openmp pthreads mpi hybrid generate_4k
#    (cuda results must be added manually from HPC cluster run)
# =============================================================================

set -uo pipefail

# ── Paths ────────────────────────────────────────────────────────────────────
BUILD="./build"
DATA="./data"
RESULTS="./results"
SERIAL_BIN="$BUILD/sobel_serial"
OPENMP_BIN="$BUILD/sobel_openmp"
PTHREADS_BIN="$BUILD/sobel_pthreads"
MPI_BIN="$BUILD/sobel_mpi"
HYBRID_BIN="$BUILD/sobel_hybrid"

SMALL_IMG="$DATA/lenna.png"        # 512×512
LARGE_IMG="$DATA/image_4k.png"     # 3840×2160

NUM_RUNS=3  # average over this many runs per config

mkdir -p "$RESULTS"

# ── CSV headers ──────────────────────────────────────────────────────────────
TIMING_CSV="$RESULTS/benchmark_results.csv"
RMSE_CSV="$RESULTS/rmse_results.csv"

echo "implementation,image_size,threads_procs,run,time_ms" > "$TIMING_CSV"
echo "implementation,image_size,threads_procs,rmse,psnr_db" > "$RMSE_CSV"

# ── Helper: extract timing (ms) from program stdout ──────────────────────────
extract_time() {
    # Looks for lines like:
    #   "Execution time: 42.3 ms"
    #   "MPI Execution time: 42.3 ms"
    #   "Hybrid Execution time: 42.3 ms"
    grep -oE '[0-9]+(\.[0-9]+)? ms' | head -1 | grep -oE '[0-9]+(\.[0-9]+)?'
}

# ── Helper: extract RMSE from stdout ─────────────────────────────────────────
extract_rmse() {
    grep "RMSE vs reference:" | grep -oE '[0-9]+(\.[0-9]+)?' | head -1
}

extract_psnr() {
    grep "PSNR:" | grep -oE '[0-9]+(\.[0-9]+)?' | head -1
}

# ── Generate reference serial outputs (once per image size) ──────────────────
generate_serial_reference() {
    local img="$1"
    local label="$2"
    local ref_out="$DATA/ref_serial_${label}.png"
    if [[ ! -f "$ref_out" ]]; then
        echo "  [ref] Generating serial reference for $label..."
        "$SERIAL_BIN" "$img" "$ref_out" serial 2>/dev/null
    fi
    echo "$ref_out"
}

# ── Run a binary N times, record each timing ─────────────────────────────────
run_n_times() {
    local impl="$1"
    local img_label="$2"
    local threads="$3"
    local cmd=("${@:4}")  # rest is the command

    for run in $(seq 1 "$NUM_RUNS"); do
        local out
        out=$("${cmd[@]}" 2>/dev/null || true)
        local t
        t=$(echo "$out" | extract_time || true)
        if [[ -n "$t" ]]; then
            echo "$impl,$img_label,$threads,$run,$t" >> "$TIMING_CSV"
        fi
    done
}

# ── Check binaries exist ──────────────────────────────────────────────────────
check_bin() {
    if [[ ! -f "$1" ]]; then
        echo "  [SKIP] Binary not found: $1"
        return 1
    fi
    return 0
}

echo "======================================================================"
echo "  Parallel Sobel Benchmark — $(date)"
echo "  Runs per config: $NUM_RUNS"
echo "======================================================================"

# =============================================================================
#  Generate 4K image if not present
# =============================================================================
if [[ ! -f "$LARGE_IMG" ]]; then
    echo ""
    echo ">>> Generating 4K test image..."
    if check_bin "$BUILD/generate_4k"; then
        "$BUILD/generate_4k"
    else
        echo "  [WARN] generate_4k not built — skipping 4K benchmarks"
    fi
fi

# =============================================================================
#  Iterate over image sizes
# =============================================================================
for IMG_LABEL in "512x512" "4K"; do

    if [[ "$IMG_LABEL" == "512x512" ]]; then
        IMG="$SMALL_IMG"
    else
        IMG="$LARGE_IMG"
    fi

    if [[ ! -f "$IMG" ]]; then
        echo "  [SKIP] Image not found: $IMG"
        continue
    fi

    echo ""
    echo "======================================================================"
    echo "  Image: $IMG_LABEL ($IMG)"
    echo "======================================================================"

    # Generate serial reference output for RMSE comparison
    REF_IMG=$(generate_serial_reference "$IMG" "$IMG_LABEL")

    # ── 1. SERIAL ─────────────────────────────────────────────────────────────
    if check_bin "$SERIAL_BIN"; then
        echo ""
        echo ">>> Serial ($IMG_LABEL)"
        run_n_times "serial" "$IMG_LABEL" "1" \
            "$SERIAL_BIN" "$IMG" "$DATA/out_serial_${IMG_LABEL}.png" serial
        echo "    Done."
    fi

    # ── 2. OPENMP ─────────────────────────────────────────────────────────────
    if check_bin "$OPENMP_BIN"; then
        echo ""
        echo ">>> OpenMP ($IMG_LABEL)"
        for T in 1 2 4 8; do
            echo "  threads=$T"
            OUT="$DATA/out_openmp_${IMG_LABEL}_t${T}.png"
            export OMP_NUM_THREADS=$T
            run_n_times "openmp" "$IMG_LABEL" "$T" \
                "$OPENMP_BIN" "$IMG" "$OUT" openmp "$T"

            # RMSE (single run)
            result=$(OMP_NUM_THREADS=$T "$OPENMP_BIN" "$IMG" "$OUT" openmp "$T" 2>/dev/null || true)
            # For OpenMP/Pthreads, RMSE is computed by comparing output images with python
        done
        unset OMP_NUM_THREADS
    fi

    # ── 3. PTHREADS ───────────────────────────────────────────────────────────
    if check_bin "$PTHREADS_BIN"; then
        echo ""
        echo ">>> Pthreads ($IMG_LABEL)"
        for T in 1 2 4 8; do
            echo "  threads=$T"
            OUT="$DATA/out_pthreads_${IMG_LABEL}_t${T}.png"
            run_n_times "pthreads" "$IMG_LABEL" "$T" \
                "$PTHREADS_BIN" "$IMG" "$OUT" pthreads "$T"
        done
    fi

    # ── 4. MPI ────────────────────────────────────────────────────────────────
    if check_bin "$MPI_BIN"; then
        echo ""
        echo ">>> MPI ($IMG_LABEL)"
        for NP in 1 2 4 8; do
            echo "  processes=$NP"
            OUT="$DATA/out_mpi_${IMG_LABEL}_np${NP}.png"
            run_n_times "mpi" "$IMG_LABEL" "$NP" \
                mpirun -np "$NP" "$MPI_BIN" "$IMG" "$OUT"

            # RMSE for MPI (uses built-in comparison with reference)
            mpi_out=$(mpirun -np "$NP" "$MPI_BIN" "$IMG" "$OUT" "$REF_IMG" 2>/dev/null || true)
            rmse=$(echo "$mpi_out" | extract_rmse)
            psnr=$(echo "$mpi_out" | extract_psnr)
            if [[ -n "$rmse" ]]; then
                echo "mpi,$IMG_LABEL,$NP,$rmse,$psnr" >> "$RMSE_CSV"
            fi
        done
    fi

    # ── 5. HYBRID (MPI + OpenMP) ──────────────────────────────────────────────
    if check_bin "$HYBRID_BIN"; then
        echo ""
        echo ">>> Hybrid MPI+OpenMP ($IMG_LABEL)"
        # Configurations: (processes × threads_per_proc)
        # Total parallel units: 2x2=4, 2x4=8, 4x2=8, 1x8=8
        for config in "2x2" "2x4" "4x2" "1x8"; do
            NP="${config%%x*}"
            OT="${config##*x}"
            label="${NP}proc_${OT}thr"
            echo "  $config ($NP processes × $OT threads)"
            OUT="$DATA/out_hybrid_${IMG_LABEL}_${label}.png"
            export OMP_NUM_THREADS=$OT
            run_n_times "hybrid" "$IMG_LABEL" "$label" \
                mpirun -np "$NP" "$HYBRID_BIN" "$IMG" "$OUT"

            # RMSE for Hybrid
            hybrid_out=$(OMP_NUM_THREADS=$OT mpirun -np "$NP" "$HYBRID_BIN" "$IMG" "$OUT" "$REF_IMG" 2>/dev/null || true)
            rmse=$(echo "$hybrid_out" | extract_rmse)
            psnr=$(echo "$hybrid_out" | extract_psnr)
            if [[ -n "$rmse" ]]; then
                echo "hybrid,$IMG_LABEL,$label,$rmse,$psnr" >> "$RMSE_CSV"
            fi
        done
        unset OMP_NUM_THREADS
    fi

done

echo ""
echo "======================================================================"
echo "  Benchmark complete!"
echo "  Timing data : $TIMING_CSV"
echo "  Accuracy    : $RMSE_CSV"
echo "======================================================================"

# =============================================================================
#  Compute RMSE for OpenMP and Pthreads outputs using Python (post-processing)
#  This runs after all images are saved.
# =============================================================================
echo ""
echo ">>> Computing RMSE for OpenMP and Pthreads outputs..."

python3 - <<'PYEOF'
import os, math
try:
    from PIL import Image
    import numpy as np
except ImportError:
    print("  [WARN] PIL/numpy not installed. Skipping RMSE for OpenMP/Pthreads.")
    print("         Install with: pip3 install Pillow numpy")
    exit(0)

DATA = "./data"
OUT_CSV = "./results/rmse_results.csv"

def rmse(a, b):
    a = np.array(a, dtype=np.float64)
    b = np.array(b, dtype=np.float64)
    return math.sqrt(np.mean((a - b) ** 2))

def psnr(r):
    return float('inf') if r == 0 else 20 * math.log10(255.0 / r)

refs = {}
for label in ["512x512", "4K"]:
    ref_path = f"{DATA}/ref_serial_{label}.png"
    if os.path.exists(ref_path):
        refs[label] = np.array(Image.open(ref_path).convert("L"))

rows = []
for label in ["512x512", "4K"]:
    if label not in refs:
        continue
    ref = refs[label]
    # OpenMP
    for T in [1, 2, 4, 8]:
        path = f"{DATA}/out_openmp_{label}_t{T}.png"
        if os.path.exists(path):
            img = np.array(Image.open(path).convert("L"))
            if img.shape == ref.shape:
                r = rmse(ref, img)
                p = psnr(r)
                rows.append(f"openmp,{label},{T},{r:.6f},{p:.2f}")
    # Pthreads
    for T in [1, 2, 4, 8]:
        path = f"{DATA}/out_pthreads_{label}_t{T}.png"
        if os.path.exists(path):
            img = np.array(Image.open(path).convert("L"))
            if img.shape == ref.shape:
                r = rmse(ref, img)
                p = psnr(r)
                rows.append(f"pthreads,{label},{T},{r:.6f},{p:.2f}")

if rows:
    with open(OUT_CSV, "a") as f:
        for row in rows:
            f.write(row + "\n")
    print(f"  Added {len(rows)} RMSE entries to {OUT_CSV}")
else:
    print("  No OpenMP/Pthreads output images found to compute RMSE.")
PYEOF

echo ""
echo "All done. Check results/ directory for CSV output."
