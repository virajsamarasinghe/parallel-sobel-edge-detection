#include <cuda_runtime.h>
#include <vector>
#include <iostream>
#include <cmath>
#include <cstdint>

// CUDA kernel to apply Sobel filter on GPU
__global__ void sobel_cuda_kernel(const uint8_t* __restrict__ input,
                                  uint8_t* __restrict__ output,
                                  int width, int height) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x >= 0 && x < width && y >= 0 && y < height) {
        // If it's a border pixel, set to 0 (ignore)
        if (x == 0 || x == width - 1 || y == 0 || y == height - 1) {
            output[y * width + x] = 0;
            return;
        }

        const int Gx[3][3] = {
            {-1, 0, 1},
            {-2, 0, 2},
            {-1, 0, 1}
        };

        const int Gy[3][3] = {
            { 1, 2, 1},
            { 0, 0, 0},
            {-1,-2,-1}
        };

        int sumX = 0;
        int sumY = 0;

        for (int j = -1; j <= 1; ++j) {
            for (int i = -1; i <= 1; ++i) {
                int pixel = input[(y + j) * width + (x + i)];
                sumX += pixel * Gx[j + 1][i + 1];
                sumY += pixel * Gy[j + 1][i + 1];
            }
        }

        int mag = static_cast<int>(sqrtf(sumX * sumX + sumY * sumY));
        output[y * width + x] = static_cast<uint8_t>(mag > 255 ? 255 : mag);
    }
}

// Wrapper function to be called from host (C++ code)
void sobel_cuda(const std::vector<uint8_t>& input,
                std::vector<uint8_t>& output,
                int width, int height) {
    size_t size = width * height * sizeof(uint8_t);

    uint8_t* d_input = nullptr;
    uint8_t* d_output = nullptr;

    // Allocate device memory
    cudaError_t err = cudaMalloc(&d_input, size);
    if (err != cudaSuccess) {
        std::cerr << "CUDA malloc failed for input: " << cudaGetErrorString(err) << std::endl;
        return;
    }

    err = cudaMalloc(&d_output, size);
    if (err != cudaSuccess) {
        std::cerr << "CUDA malloc failed for output: " << cudaGetErrorString(err) << std::endl;
        cudaFree(d_input);
        return;
    }

    // Copy input data to device
    err = cudaMemcpy(d_input, input.data(), size, cudaMemcpyHostToDevice);
    if (err != cudaSuccess) {
        std::cerr << "CUDA memcpy H2D failed: " << cudaGetErrorString(err) << std::endl;
        cudaFree(d_input);
        cudaFree(d_output);
        return;
    }

    // Set output memory to 0
    cudaMemset(d_output, 0, size);

    // Launch configuration
    dim3 blockSize(16, 16);
    dim3 gridSize((width + blockSize.x - 1) / blockSize.x,
                  (height + blockSize.y - 1) / blockSize.y);

    // Launch CUDA kernel
    sobel_cuda_kernel<<<gridSize, blockSize>>>(d_input, d_output, width, height);

    // Check for kernel launch errors
    err = cudaGetLastError();
    if (err != cudaSuccess) {
        std::cerr << "CUDA kernel launch failed: " << cudaGetErrorString(err) << std::endl;
        cudaFree(d_input);
        cudaFree(d_output);
        return;
    }

    // Wait for GPU to finish
    err = cudaDeviceSynchronize();
    if (err != cudaSuccess) {
        std::cerr << "CUDA device synchronize failed: " << cudaGetErrorString(err) << std::endl;
        cudaFree(d_input);
        cudaFree(d_output);
        return;
    }

    // Copy output data back to host
    err = cudaMemcpy(output.data(), d_output, size, cudaMemcpyDeviceToHost);
    if (err != cudaSuccess) {
        std::cerr << "CUDA memcpy D2H failed: " << cudaGetErrorString(err) << std::endl;
    }

    // Free device memory
    cudaFree(d_input);
    cudaFree(d_output);
}
