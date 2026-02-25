# Phase 4: Hybrid & GPU Acceleration

**Duration:** Weeks 7-8

This phase combines approaches and utilizes specialized hardware for maximum performance using C++ and CUDA.

## Objectives
*   **Step 1: Hybrid MPI + OpenMP.** Modify the MPI C++ code so that each worker process utilizes OpenMP to spawn internal threads. Ensure MPI is initialized with thread support using `MPI_Init_thread`. Compile with both `mpicxx` and `-fopenmp`.
*   **Step 2: GPU Implementation (CUDA/OpenCL).** Write a CUDA C++ kernel (`.cu` file) for the Sobel operator. Allocate device memory (`cudaMalloc`), transfer the `std::vector` image from host CPU to GPU (`cudaMemcpy`), execute the kernel grid/blocks, and transfer the result back. Compile using `nvcc`.
*   **Step 3: Verification.** Run the RMSE and PSNR checks on both the Hybrid and GPU outputs. Use CUDA event timers (`cudaEventRecord`) to profile GPU execution time accurately without host-device synchronization overhead.
