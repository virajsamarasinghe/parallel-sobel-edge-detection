# Phase 3: Distributed-Memory Parallelization

**Duration:** Weeks 5-6

This phase scales the application across multiple nodes using the Message Passing Interface (MPI) with the `mpicxx` compiler wrapper.

## Objectives
*   **Step 1: MPI Environment.** Initialize the MPI environment (`MPI_Init`, `MPI_Comm_rank`, `MPI_Comm_size`) and establish the Master-Worker paradigm within your C++ application.
*   **Step 2: Domain Decomposition.** Program the Master process (Rank 0) to divide the 4096×4096 `std::vector` image into horizontal or vertical blocks for the workers.
*   **Step 3: Halo Exchange (Ghost Cells).** Implement point-to-point communication (`MPI_Send`/`MPI_Recv` or `MPI_Isend`/`MPI_Irecv`) to exchange the 1-pixel boundary rows (halo regions) between neighboring processes, which is required by the 3x3 Sobel kernel.
*   **Step 4: Scatter and Gather.** Distribute the image chunks using `MPI_Scatterv` and collect the processed edge-detected chunks back to the Master using `MPI_Gatherv`. Use `MPI_Wtime()` for benchmarking distributed execution.
*   **Step 5: Verification.** Compare the gathered MPI output against the serial baseline using RMSE/PSNR.
