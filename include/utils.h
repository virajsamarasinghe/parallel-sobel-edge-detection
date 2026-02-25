#ifndef UTILS_H
#define UTILS_H

#include <cstdint>
#include <vector>

double calculate_rmse(const std::vector<uint8_t> &img1,
                      const std::vector<uint8_t> &img2);

double calculate_psnr(double rmse);

#endif // UTILS_H
