#include <chrono>
#include <iostream>
#include <string>
#include <vector>

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../include/stb/stb_image_write.h"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include "../include/sobel.h"
#include "../include/utils.h"

int main(int argc, char **argv) {
  if (argc < 3) {
    std::cerr
        << "Usage: " << argv[0]
        << " <input_image_path> <output_image_path> [reference_image_path]"
        << std::endl;
    return 1;
  }

  std::string input_path = argv[1];
  std::string output_path = argv[2];

  int width, height, channels;
  // Force loaded image to be grayscale (1 channel)
  uint8_t *img_data =
      stbi_load(input_path.c_str(), &width, &height, &channels, 1);

  if (!img_data) {
    std::cerr << "Error: Failed to load image " << input_path << std::endl;
    return 1;
  }

  std::cout << "Loaded image: " << width << "x" << height << std::endl;

  size_t img_size = static_cast<size_t>(width) * height;
  std::vector<uint8_t> input_img(img_data, img_data + img_size);
  std::vector<uint8_t> output_img(
      img_size, 0); // Initialize with 0 (black background for borders)

  stbi_image_free(img_data);

  std::cout << "Starting serial Sobel edge detection..." << std::endl;

  auto start_time = std::chrono::high_resolution_clock::now();

  sobel_serial(input_img, output_img, width, height);

  auto end_time = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> duration_ms = end_time - start_time;

  std::cout << "Execution time: " << duration_ms.count() << " ms" << std::endl;

  int stride_in_bytes = width; // 1 byte per pixel for grayscale
  if (!stbi_write_png(output_path.c_str(), width, height, 1, output_img.data(),
                      stride_in_bytes)) {
    std::cerr << "Error: Failed to write output image to " << output_path
              << std::endl;
    return 1;
  }

  std::cout << "Output saved to " << output_path << std::endl;

  if (argc >= 4) {
    std::string ref_path = argv[3];
    int ref_width, ref_height, ref_channels;
    uint8_t *ref_data =
        stbi_load(ref_path.c_str(), &ref_width, &ref_height, &ref_channels, 1);

    if (ref_data) {
      if (ref_width == width && ref_height == height) {
        std::vector<uint8_t> ref_img(ref_data,
                                     ref_data + (ref_width * ref_height));
        double rmse = calculate_rmse(output_img, ref_img);
        double psnr = calculate_psnr(rmse);
        std::cout << "Validation against " << ref_path << ":" << std::endl;
        std::cout << "  RMSE: " << rmse << std::endl;
        if (psnr < 0) {
          std::cout << "  PSNR: Infinity (Perfect match)" << std::endl;
        } else {
          std::cout << "  PSNR: " << psnr << " dB" << std::endl;
        }
      } else {
        std::cerr << "Validation Error: Reference image dimensions ("
                  << ref_width << "x" << ref_height
                  << ") do not match output dimensions." << std::endl;
      }
      stbi_image_free(ref_data);
    } else {
      std::cerr << "Validation Error: Could not load reference image "
                << ref_path << std::endl;
    }
  }

  return 0;
}
