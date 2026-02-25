#ifndef SOBEL_H
#define SOBEL_H

#include <cstdint>
#include <vector>

void sobel_serial(const std::vector<uint8_t> &input,
                  std::vector<uint8_t> &output, int width, int height);

#endif // SOBEL_H
