"""
Generate all performance analysis charts for the HPC Sobel report.
Saves PNGs to docs/hpc/diagram/.
"""
import os
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np

OUT = os.path.join(os.path.dirname(__file__), "diagram")
os.makedirs(OUT, exist_ok=True)

# ── Style ─────────────────────────────────────────────────────────────────────
plt.rcParams.update({
    'font.family': 'DejaVu Sans',
    'font.size': 10,
    'axes.titlesize': 11,
    'axes.labelsize': 10,
    'xtick.labelsize': 9,
    'ytick.labelsize': 9,
    'axes.grid': True,
    'grid.alpha': 0.4,
    'axes.spines.top': False,
    'axes.spines.right': False,
    'figure.dpi': 150,
})

C_OMP   = '#2196F3'   # blue
C_PTH   = '#4CAF50'   # green
C_MPI   = '#9C27B0'   # purple
C_HYB   = '#FF9800'   # amber
C_CUDA  = '#FF5722'   # red-orange
C_SER   = '#607D8B'   # grey
C_IDEAL = '#BDBDBD'   # light grey (ideal line)

# ══════════════════════════════════════════════════════════════════════════════
# Chart 1 — Speedup vs Threads (4K), OpenMP + Pthreads
# ══════════════════════════════════════════════════════════════════════════════
fig, ax = plt.subplots(figsize=(6, 4))
threads = [1, 2, 4, 8]
omp_sp  = [0.33, 0.63, 1.14, 1.47]
pth_sp  = [0.93, 1.78, 3.03, 3.10]
ideal   = [1.0,  2.0,  4.0,  8.0 ]

ax.plot(threads, ideal,  '--', color=C_IDEAL, lw=1.5, label='Ideal linear', zorder=1)
ax.plot(threads, omp_sp, '-o', color=C_OMP,   lw=2, ms=7, label='OpenMP',   zorder=3)
ax.plot(threads, pth_sp, '-s', color=C_PTH,   lw=2, ms=7, label='Pthreads', zorder=3)

for x, y in zip(threads, omp_sp):
    ax.annotate(f'{y:.2f}×', (x, y), textcoords='offset points',
                xytext=(0, 8), ha='center', fontsize=8, color=C_OMP)
for x, y in zip(threads, pth_sp):
    ax.annotate(f'{y:.2f}×', (x, y), textcoords='offset points',
                xytext=(0, 8), ha='center', fontsize=8, color=C_PTH)

ax.set_xlabel('Number of Threads')
ax.set_ylabel('Speedup (×)')
ax.set_title('Speedup vs Threads — 4K Image (3840×2160)\n'
             'Serial baseline = 20.22 ms', fontsize=10)
ax.set_xticks(threads)
ax.legend(framealpha=0.8, fontsize=9)
plt.tight_layout()
plt.savefig(os.path.join(OUT, 'speedup_shared_4k.png'))
plt.close()

# ══════════════════════════════════════════════════════════════════════════════
# Chart 2 — Speedup vs Threads (4096×4096), OpenMP + Pthreads
# ══════════════════════════════════════════════════════════════════════════════
fig, ax = plt.subplots(figsize=(6, 4))
serial_4096 = 43.45
omp_t_4096  = [164.48, 95.36, 74.24, 69.12]
pth_t_4096  = [49.28,  26.24, 16.00, 18.56]
omp_sp_4096 = [serial_4096/t for t in omp_t_4096]
pth_sp_4096 = [serial_4096/t for t in pth_t_4096]

ax.plot(threads, ideal,       '--', color=C_IDEAL, lw=1.5, label='Ideal linear', zorder=1)
ax.plot(threads, omp_sp_4096, '-o', color=C_OMP,   lw=2, ms=7, label='OpenMP',   zorder=3)
ax.plot(threads, pth_sp_4096, '-s', color=C_PTH,   lw=2, ms=7, label='Pthreads', zorder=3)

for x, y in zip(threads, omp_sp_4096):
    ax.annotate(f'{y:.2f}×', (x, y), textcoords='offset points',
                xytext=(0, 8), ha='center', fontsize=8, color=C_OMP)
for x, y in zip(threads, pth_sp_4096):
    ax.annotate(f'{y:.2f}×', (x, y), textcoords='offset points',
                xytext=(0, 8), ha='center', fontsize=8, color=C_PTH)

ax.set_xlabel('Number of Threads')
ax.set_ylabel('Speedup (×)')
ax.set_title('Speedup vs Threads — 4096×4096 Lenna Image\n'
             'Serial baseline = 43.45 ms', fontsize=10)
ax.set_xticks(threads)
ax.legend(framealpha=0.8, fontsize=9)
plt.tight_layout()
plt.savefig(os.path.join(OUT, 'speedup_512.png'))
plt.close()

# ══════════════════════════════════════════════════════════════════════════════
# Chart 3 — MPI Execution Time (4K)
# ══════════════════════════════════════════════════════════════════════════════
fig, ax = plt.subplots(figsize=(5.5, 4))
procs    = [1, 2, 4, 8]
mpi_t    = [9.38, 5.23, 4.08, 10.33]
bars = ax.bar([str(p) for p in procs], mpi_t, color=C_MPI, alpha=0.85,
              edgecolor='white', width=0.55)
for bar, val in zip(bars, mpi_t):
    ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.2,
            f'{val:.2f} ms', ha='center', va='bottom', fontsize=9)
ax.axhline(y=9.38, color=C_SER, linestyle='--', lw=1.4, alpha=0.7,
           label='MPI-1P baseline (9.38 ms)')
ax.set_xlabel('Number of MPI Processes')
ax.set_ylabel('Execution Time (ms)')
ax.set_title('MPI Execution Time — 4K Image\n(timing: MPI_Wtime, excludes image I/O)', fontsize=10)
ax.legend(fontsize=8, framealpha=0.8)
ax.set_ylim(0, 13)
plt.tight_layout()
plt.savefig(os.path.join(OUT, 'mpi_time_4k.png'))
plt.close()

# ══════════════════════════════════════════════════════════════════════════════
# Chart 4 — Hybrid Execution Time (4K)
# ══════════════════════════════════════════════════════════════════════════════
fig, ax = plt.subplots(figsize=(6, 4))
configs  = ['2P×2T\n(4 units)', '2P×4T\n(8 units)', '4P×2T\n(8 units)', '1P×8T\n(8 units)']
hyb_t    = [18.96, 16.33, 15.17, 17.22]
bars = ax.bar(configs, hyb_t, color=C_HYB, alpha=0.85,
              edgecolor='white', width=0.55)
for bar, val in zip(bars, hyb_t):
    ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.2,
            f'{val:.2f}', ha='center', va='bottom', fontsize=9)
ax.axhline(y=20.22, color=C_SER,  linestyle='--', lw=1.5, label='Serial (20.22 ms)')
ax.axhline(y=4.08,  color=C_MPI, linestyle=':',  lw=1.5, label='Best MPI (4.08 ms)')
ax.set_ylabel('Execution Time (ms)')
ax.set_title('Hybrid MPI+OpenMP Execution Time — 4K Image', fontsize=10)
ax.legend(fontsize=8, framealpha=0.8)
ax.set_ylim(0, 25)
plt.tight_layout()
plt.savefig(os.path.join(OUT, 'hybrid_time_4k.png'))
plt.close()

# ══════════════════════════════════════════════════════════════════════════════
# Chart 5 — Parallel Efficiency (4K), OpenMP + Pthreads
# ══════════════════════════════════════════════════════════════════════════════
fig, ax = plt.subplots(figsize=(6, 4))
omp_eff = [s/t for s, t in zip(omp_sp, threads)]
pth_eff = [s/t for s, t in zip(pth_sp, threads)]

ax.axhline(y=1.0, color=C_IDEAL, linestyle='--', lw=1.5, label='Ideal (E = 1.0)', zorder=1)
ax.plot(threads, omp_eff, '-o', color=C_OMP, lw=2, ms=7, label='OpenMP',   zorder=3)
ax.plot(threads, pth_eff, '-s', color=C_PTH, lw=2, ms=7, label='Pthreads', zorder=3)

for x, y in zip(threads, omp_eff):
    ax.annotate(f'{y:.2f}', (x, y), textcoords='offset points',
                xytext=(0, 8), ha='center', fontsize=8, color=C_OMP)
for x, y in zip(threads, pth_eff):
    ax.annotate(f'{y:.2f}', (x, y), textcoords='offset points',
                xytext=(0, 8), ha='center', fontsize=8, color=C_PTH)

ax.set_xlabel('Number of Threads')
ax.set_ylabel('Efficiency  (Speedup / Threads)')
ax.set_title('Parallel Efficiency — 4K Image', fontsize=10)
ax.set_xticks(threads)
ax.set_ylim(0, 1.2)
ax.legend(framealpha=0.8, fontsize=9)
plt.tight_layout()
plt.savefig(os.path.join(OUT, 'efficiency_4k.png'))
plt.close()

# ══════════════════════════════════════════════════════════════════════════════
# Chart 6 — Best Execution Time Comparison (4K), horizontal bars
# ══════════════════════════════════════════════════════════════════════════════
fig, ax = plt.subplots(figsize=(7, 4.5))
labels  = ['Serial', 'OpenMP\n(8 threads)', 'Pthreads\n(4 threads)',
           'MPI\n(4 processes)', 'Hybrid\n(4P×2T)', 'CUDA\n(GPU)']
times   = [20.22, 13.80, 6.67, 4.08, 15.17, 6.289]
colors  = [C_SER, C_OMP, C_PTH, C_MPI, C_HYB, C_CUDA]

y_pos = np.arange(len(labels))
for i, (label, t, col) in enumerate(zip(labels, times, colors)):
    ax.barh(i, t, color=col, alpha=0.85, edgecolor='white', height=0.55)
    ax.text(t + 0.3, i, f'{t:.3f} ms', va='center', fontsize=9)

ax.set_yticks(y_pos)
ax.set_yticklabels(labels, fontsize=9)
ax.set_xlabel('Execution Time (ms)')
ax.set_title('Best Execution Time by Implementation — 4K Image\n'
             '(CUDA: NVIDIA GTX 960M, end-to-end incl. PCIe transfers)', fontsize=10)
ax.set_xlim(0, 27)
plt.tight_layout()
plt.savefig(os.path.join(OUT, 'exec_time_comparison_4k.png'))
plt.close()

# ══════════════════════════════════════════════════════════════════════════════
# Chart 7 — RMSE bar chart (all zero)
# ══════════════════════════════════════════════════════════════════════════════
fig, ax = plt.subplots(figsize=(6.5, 4))
impls  = ['OpenMP', 'Pthreads', 'MPI', 'Hybrid\nMPI+OMP', 'CUDA']
rmse   = [0.0, 0.0, 0.0, 0.0, 0.0]
cols   = [C_OMP, C_PTH, C_MPI, C_HYB, C_CUDA]

bars = ax.bar(impls, rmse, color=cols, alpha=0.85, edgecolor='white', width=0.55)
for bar in bars:
    ax.text(bar.get_x() + bar.get_width()/2, 0.00008,
            '0.0000', ha='center', va='bottom', fontsize=9, fontweight='bold')
ax.set_ylim(0, 0.0012)
ax.set_ylabel('RMSE (vs serial reference)')
ax.set_title('Accuracy — RMSE vs Serial for All Implementations\n'
             'Both image sizes: 4096×4096 and 4K', fontsize=10)
plt.tight_layout()
plt.savefig(os.path.join(OUT, 'rmse_bar.png'))
plt.close()

# ══════════════════════════════════════════════════════════════════════════════
# Chart 8 — MPI internal speedup (4K)
# ══════════════════════════════════════════════════════════════════════════════
fig, ax = plt.subplots(figsize=(5.5, 4))
mpi_1p     = 9.38
mpi_sp_int = [mpi_1p / t for t in mpi_t]  # internal speedup vs MPI-1P
ideal_mpi  = [1.0, 2.0, 4.0, 8.0]
ax.plot(procs, ideal_mpi,  '--', color=C_IDEAL, lw=1.5, label='Ideal linear', zorder=1)
ax.plot(procs, mpi_sp_int, '-D', color=C_MPI,   lw=2, ms=7, label='MPI (vs MPI-1P)', zorder=3)
for x, y in zip(procs, mpi_sp_int):
    ax.annotate(f'{y:.2f}×', (x, y), textcoords='offset points',
                xytext=(0, 8), ha='center', fontsize=8, color=C_MPI)
ax.set_xlabel('Number of MPI Processes')
ax.set_ylabel('Speedup (×)')
ax.set_title('MPI Speedup — 4K Image\n(relative to MPI 1-process baseline)', fontsize=10)
ax.set_xticks(procs)
ax.legend(framealpha=0.8, fontsize=9)
plt.tight_layout()
plt.savefig(os.path.join(OUT, 'mpi_speedup_4k.png'))
plt.close()

print("All charts saved:")
for f in sorted(os.listdir(OUT)):
    if f.endswith('.png'):
        print(f"  {os.path.join(OUT, f)}")
