
// sobel_openmp.cpp  –  OpenMP parallel Sobel edge detection


#include "../include/sobel.h"
#include <algorithm>
#include <cmath>

#ifdef _OPENMP
#include <omp.h>
#endif

void sobel_openmp(const std::vector<uint8_t> &input,
                  std::vector<uint8_t> &output,
                  int width, int height) {

    if (width < 3 || height < 3) return;

    // Zero border pixels (single-threaded, tiny work)
    for (int x = 0; x < width; ++x) {
        output[0 * width + x]            = 0;
        output[(height - 1) * width + x] = 0;
    }
    for (int y = 0; y < height; ++y) {
        output[y * width + 0]           = 0;
        output[y * width + (width - 1)] = 0;
    }

    // Parallel row loop
    // Each thread owns a contiguous stripe of rows – no shared writes.
#pragma omp parallel for schedule(static) default(none) \
    shared(input, output, width, height)
    for (int y = 1; y < height - 1; ++y) {

        // Thread-private kernel copies (stack allocation – fast, no sharing)
        const int Gx[3][3] = {{-1, 0, 1},
                               {-2, 0, 2},
                               {-1, 0, 1}};

        const int Gy[3][3] = {{ 1,  2,  1},
                               { 0,  0,  0},
                               {-1, -2, -1}};

        for (int x = 1; x < width - 1; ++x) {
            int sumX = 0;
            int sumY = 0;

            for (int j = -1; j <= 1; ++j) {
                for (int i = -1; i <= 1; ++i) {
                    int pixel_val = static_cast<int>(
                        input[(y + j) * width + (x + i)]);
                    sumX += pixel_val * Gx[j + 1][i + 1];
                    sumY += pixel_val * Gy[j + 1][i + 1];
                }
            }

            int magnitude = static_cast<int>(std::sqrt(
                static_cast<double>(sumX * sumX + sumY * sumY)));

            output[y * width + x] =
                static_cast<uint8_t>(std::min(255, magnitude));
        }
    }
}