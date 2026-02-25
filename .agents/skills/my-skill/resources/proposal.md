# Parallel 2D Image Convolution for Edge Detection

## Problem Statement
Image convolution is a fundamental operation in digital image processing, widely used for edge detection, blurring, and feature extraction. Applying a 3×3 Sobel kernel to a 4096×4096 image requires over 150 million arithmetic operations, making sequential implementations a significant bottleneck for real-time processing. 

This project develops and benchmarks parallel implementations of 2D Sobel edge detection using OpenMP, Pthreads, MPI, and GPU programming to evaluate speedup, scalability, and accuracy compared to a serial baseline.

## Objectives
1. Implement a serial 2D image convolution algorithm for Sobel edge detection as the baseline reference.
2. Develop parallel versions using OpenMP, Pthreads, MPI, Hybrid MPI+OpenMP, and GPU.
3. Verify correctness of all parallel implementations against the serial output using RMSE and PSNR metrics.
4. Conduct comprehensive performance analysis measuring execution time, speedup, and parallel efficiency.

## Tech Stack & MPI Implementation Details
Based on standard High-Performance Computing practices from Lawrence Livermore National Laboratory (LLNL), the distributed-memory parallelization of this project will utilize the following stack and methodologies:

*   **Language & Compilers:** The project will be implemented in C/C++ [page:2]. Compilation will be handled by MPI wrapper scripts such as `mpicc` (for C) or `mpicxx` (for C++) [page:2].
*   **MPI Implementations:** Testing and benchmarking will be compatible with standard Linux cluster implementations like **Open MPI** or **MVAPICH** [page:2].
*   **Execution:** Distributed jobs will be launched using `mpirun`, `mpiexec`, or `srun` across multiple cluster nodes [page:2].
*   **Environment Management:** The program will utilize core MPI routines (`MPI_Init`, `MPI_Comm_rank`, `MPI_Comm_size`, and `MPI_Finalize`) to set up the communicator and identify the master and worker nodes [page:3].
*   **Domain Decomposition & Point-to-Point Communication:** The 4096×4096 image will be divided into chunks and distributed among worker nodes. To compute the Sobel kernel at the boundaries of these chunks, the project will utilize point-to-point communication (`MPI_Send` and `MPI_Recv`) to exchange "ghost cells" (halo regions) between neighboring processes [page:3]. 
*   **Master-Worker Paradigm:** Following the structure of LLNL's Pi calculation example (`mpi_pi_send.c`), the Master task (Rank 0) will be responsible for distributing the initial image data to the workers and gathering/reducing the final processed edge-detected image data once all worker computations are complete [page:3].

## References
1. Barney, B. *Message Passing Interface (MPI)*. Lawrence Livermore National Laboratory (LLNL) HPC Tutorials. Available at: [https://hpc-tutorials.llnl.gov/mpi/](https://hpc-tutorials.llnl.gov/mpi/)
2. Barney, B. *LLNL MPI Implementations and Compilers*. Lawrence Livermore National Laboratory (LLNL) HPC Tutorials. Available at: [https://hpc-tutorials.llnl.gov/mpi/implementations/](https://hpc-tutorials.llnl.gov/mpi/implementations/)
3. Barney, B. *MPI pi Calculation Example - C Version (mpi_pi_send.c)*. Lawrence Livermore National Laboratory (LLNL) HPC Tutorials. Available at: [https://hpc-tutorials.llnl.gov/mpi/examples/mpi_pi_send.c](https://hpc-tutorials.llnl.gov/mpi/examples/mpi_pi_send.c)
