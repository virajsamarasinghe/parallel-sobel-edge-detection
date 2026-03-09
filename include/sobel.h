#ifndef SOBEL_H
#define SOBEL_H

#include <vector>
#include <cstdint>

// Serial version (already implemented)
void sobel_serial(const std::vector<uint8_t>& input,
                  std::vector<uint8_t>& output,
                  int width, int height);

// OpenMP version
void sobel_openmp(const std::vector<uint8_t>& input,
                  std::vector<uint8_t>& output,
                  int width, int height);

// Pthreads version
void sobel_pthreads(const std::vector<uint8_t>& input,
                    std::vector<uint8_t>& output,
                    int width, int height,
                    int num_threads);

// MPI version
void sobel_mpi(const std::vector<uint8_t>& input,
               std::vector<uint8_t>& output,
               int width, int height);

#endif