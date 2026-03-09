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

// Forward declare optional functions
#ifdef HAS_OPENMP
void sobel_openmp(const std::vector<uint8_t> &input,
                  std::vector<uint8_t> &output, int width, int height);
#endif

#ifdef HAS_PTHREADS
void sobel_pthreads(const std::vector<uint8_t> &input,
                    std::vector<uint8_t> &output, int width, int height, int num_threads);
#endif

int main(int argc, char **argv) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0]
                  << " <input_image_path> <output_image_path> <mode> [num_threads] [reference_image_path]\n";
        std::cerr << "Modes: serial";
#ifdef HAS_OPENMP
        std::cerr << ", openmp";
#endif
#ifdef HAS_PTHREADS
        std::cerr << ", pthreads";
#endif
        std::cerr << std::endl;
        return 1;
    }

    std::string input_path = argv[1];
    std::string output_path = argv[2];
    std::string mode = argv[3];

    int num_threads = 4;
    if (argc >= 5) {
        num_threads = std::stoi(argv[4]);
    }

    // Load image
    int width, height, channels;
    uint8_t* img_data = stbi_load(input_path.c_str(), &width, &height, &channels, 1);
    if (!img_data) {
        std::cerr << "Error: Failed to load image " << input_path << std::endl;
        return 1;
    }
    std::vector<uint8_t> input_img(img_data, img_data + width * height);
    std::vector<uint8_t> output_img(width * height, 0);
    stbi_image_free(img_data);

    std::cout << "Loaded image: " << width << "x" << height << std::endl;
    std::cout << "Starting " << mode << " Sobel edge detection..." << std::endl;

    auto start_time = std::chrono::high_resolution_clock::now();

    if (mode == "serial") {
        sobel_serial(input_img, output_img, width, height);
    }
#ifdef HAS_OPENMP
    else if (mode == "openmp") {
        sobel_openmp(input_img, output_img, width, height);
    }
#endif
#ifdef HAS_PTHREADS
    else if (mode == "pthreads") {
        sobel_pthreads(input_img, output_img, width, height, num_threads);
    }
#endif
    else {
        std::cerr << "Unknown mode or mode not supported in this build: " << mode << std::endl;
        return 1;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration_ms = end_time - start_time;
    std::cout << "Execution time: " << duration_ms.count() << " ms" << std::endl;

    if (!stbi_write_png(output_path.c_str(), width, height, 1, output_img.data(), width)) {
        std::cerr << "Error: Failed to write output image\n";
        return 1;
    }
    std::cout << "Output saved to " << output_path << std::endl;

    return 0;
}