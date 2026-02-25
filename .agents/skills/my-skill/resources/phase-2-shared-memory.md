# Phase 2: Shared-Memory Parallelization

**Duration:** Weeks 3-4

This phase focuses on multi-threading the C++ application on a single machine (CPU) using shared memory.

## Objectives
*   **Step 1: OpenMP Implementation.** Compile with `-fopenmp` and add OpenMP pragmas (e.g., `#pragma omp parallel for`) to the outer image loops of your C++ code. Experiment with different scheduling clauses (`schedule(static)` vs `schedule(dynamic)`).
*   **Step 2: Pthreads Implementation.** Implement manual thread management using the POSIX Threads API (`<pthread.h>`). Divide the `std::vector` image rows into chunks, spawn threads using `pthread_create`, pass a struct containing pointers to the workload, and synchronize using `pthread_join`.
*   **Step 3: Verification.** Pass the outputs of both OpenMP and Pthreads through the RMSE/PSNR functions to ensure they perfectly match the serial baseline.
*   **Step 4: Single-Node Benchmarking.** Test the implementations using 2, 4, 8, and 16 threads. Use `omp_get_wtime()` for OpenMP timing and record the preliminary speedups.
