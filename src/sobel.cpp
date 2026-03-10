
// sobel_serial.cpp  –  Serial Sobel edge detection (baseline)


#include "../include/sobel.h"
#include <algorithm>
#include <cmath>

void sobel_serial(const std::vector<uint8_t> &input,
                  std::vector<uint8_t> &output,
                  int width, int height) {

    // Degenerate input guard
    if (width < 3 || height < 3) return;

    // Sobel kernels
    //  Gx detects horizontal gradients (vertical edges)
    //  Gy detects vertical gradients   (horizontal edges)
    const int Gx[3][3] = {{-1, 0, 1},
                           {-2, 0, 2},
                           {-1, 0, 1}};

    const int Gy[3][3] = {{ 1,  2,  1},
                           { 0,  0,  0},
                           {-1, -2, -1}};

    // Zero the border pixels so the output is fully defined
    for (int x = 0; x < width; ++x) {
        output[0 * width + x]            = 0;   // top row
        output[(height - 1) * width + x] = 0;   // bottom row
    }
    for (int y = 0; y < height; ++y) {
        output[y * width + 0]           = 0;    // left column
        output[y * width + (width - 1)] = 0;    // right column
    }

    // Inner pixels
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            int sumX = 0;
            int sumY = 0;

            for (int j = -1; j <= 1; ++j) {
                for (int i = -1; i <= 1; ++i) {
                    int pixel_val = static_cast<int>(input[(y + j) * width + (x + i)]);
                    sumX += pixel_val * Gx[j + 1][i + 1];
                    sumY += pixel_val * Gy[j + 1][i + 1];
                }
            }

            // L2 magnitude, clamped to [0, 255]
            int magnitude = static_cast<int>(std::sqrt(
                static_cast<double>(sumX * sumX + sumY * sumY)));

            output[y * width + x] = static_cast<uint8_t>(std::min(255, magnitude));
        }
    }
}