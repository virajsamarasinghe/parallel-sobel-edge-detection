#include "../include/utils.h"
#include <cmath>
#include <iostream>

double calculate_rmse(const std::vector<uint8_t> &img1,
                      const std::vector<uint8_t> &img2) {
    if (img1.size() != img2.size() || img1.empty()) {
        std::cerr << "Error: Image sizes mismatch or images are empty." << std::endl;
        return -1.0;
    }

    double mse = 0.0;
    size_t n = img1.size();

    for (size_t i = 0; i < n; ++i) {
        double diff = static_cast<double>(img1[i]) - static_cast<double>(img2[i]);
        mse += diff * diff;
    }

    mse /= static_cast<double>(n);
    return std::sqrt(mse);
}

double calculate_psnr(double rmse) {
    if (rmse <= 0.0) {
        // RMSE == 0 means perfect match => infinite PSNR; signal with -1.0
        return -1.0;
    }
    const double max_pixel_value = 255.0;
    return 20.0 * std::log10(max_pixel_value / rmse);
}
