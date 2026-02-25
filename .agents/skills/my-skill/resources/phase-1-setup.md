# Phase 1: Project Setup & Serial Baseline

**Duration:** Weeks 1-2

The objective of this phase is to establish a correct, unoptimized sequential C++ baseline for accuracy verification and performance comparison.

## Objectives
*   **Step 1: Environment Setup.** Set up a C++ development environment (C++11 or newer) using `g++` or `clang++`. Integrate a lightweight image library like `stb_image.h` and `stb_image_write.h` to load and save 4096×4096 images as 1D `std::vector<uint8_t>` arrays.
*   **Step 2: Serial Implementation.** Write the standard 2D convolution loop applying the 3x3 Sobel operator (both X and Y directions, calculating magnitude) over the image pixels.
*   **Step 3: Verification Metrics.** Implement C++ functions for Root Mean Square Error (RMSE) and Peak Signal-to-Noise Ratio (PSNR) to compare future parallel outputs against the serial baseline.
*   **Step 4: Baseline Profiling.** Execute the serial version and measure execution time precisely using `std::chrono::high_resolution_clock`. Log the baseline performance.
