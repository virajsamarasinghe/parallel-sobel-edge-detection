#ifndef SOBEL_H
#define SOBEL_H

#include <vector>
#include <cstdint>

/**
 * Applies the Sobel operator to an image using a serial (unoptimized) implementation.
 * 
 * @param input  1D vector representing the grayscale input image pixels (row-major).
 * @param output 1D vector to store the grayscale output image pixels.
 * @param width  Width of the image in pixels.
 * @param height Height of the image in pixels.
 */
void sobel_serial(const std::vector<uint8_t>& input, std::vector<uint8_t>& output, int width, int height);

#endif // SOBEL_H
