#ifndef UTILS_H
#define UTILS_H

#include <cstdint>
#include <vector>

/**
 * Calculates the Root Mean Square Error (RMSE) between two images.
 * @param img1 First image array.
 * @param img2 Second image array.
 * @return The RMSE value, or -1.0 if sizes do not match.
 */
double calculate_rmse(const std::vector<uint8_t> &img1,
                      const std::vector<uint8_t> &img2);

/**
 * Calculates the Peak Signal-to-Noise Ratio (PSNR) between two images.
 * @param rmse The previously calculated RMSE value.
 * @return The PSNR in dB. returns -1.0 if RMSE is exactly 0.
 */
double calculate_psnr(double rmse);

#endif // UTILS_H
