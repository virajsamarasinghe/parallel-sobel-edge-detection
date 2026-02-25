#include "../include/sobel.h"
#include <algorithm>
#include <cmath>

void sobel_serial(const std::vector<uint8_t> &input,
                  std::vector<uint8_t> &output, int width, int height) {
  const int Gx[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};

  const int Gy[3][3] = {{1, 2, 1}, {0, 0, 0}, {-1, -2, -1}};

  for (int y = 1; y < height - 1; ++y) {
    for (int x = 1; x < width - 1; ++x) {
      int sumX = 0;
      int sumY = 0;
      for (int j = -1; j <= 1; ++j) {
        for (int i = -1; i <= 1; ++i) {
          int pixel_val = input[(y + j) * width + (x + i)];
          sumX += pixel_val * Gx[j + 1][i + 1];
          sumY += pixel_val * Gy[j + 1][i + 1];
        }
      }

      int magnitude = static_cast<int>(std::sqrt(sumX * sumX + sumY * sumY));

      output[y * width + x] = static_cast<uint8_t>(std::min(255, magnitude));
    }
  }
}
