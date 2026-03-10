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

#ifdef HAS_OPENMP
#include <omp.h>
#endif

// Forward declarations for parallel implementations
#ifdef HAS_OPENMP
void sobel_openmp(const std::vector<uint8_t> &input,
                  std::vector<uint8_t> &output, int width, int height);
#endif

#ifdef HAS_PTHREADS
void sobel_pthreads(const std::vector<uint8_t> &input,
                    std::vector<uint8_t> &output, int width, int height,
                    int num_threads);
#endif

// ---------------------------------------------------------------------------
int main(int argc, char **argv) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0]
                  << " <input_image_path> <output_image_path> <mode>"
                     " [num_threads] [reference_image_path]\n";
        std::cerr << "Modes: serial";
#ifdef HAS_OPENMP
        std::cerr << ", openmp";
#endif
#ifdef HAS_PTHREADS
        std::cerr << ", pthreads";
#endif
        std::cerr << "\n\n"
                  << "Accuracy notes:\n"
                  << "  Supply a reference image (serial output) as the 5th\n"
                  << "  argument to compare RMSE and PSNR.\n"
                  << "  For bit-perfect parallel results RMSE should be 0.\n";
        return 1;
    }

    std::string input_path  = argv[1];
    std::string output_path = argv[2];
    std::string mode        = argv[3];

    int num_threads = 4;
    if (argc >= 5) num_threads = std::stoi(argv[4]);

    // -----------------------------------------------------------------------
    // Load input image as grayscale
    // -----------------------------------------------------------------------
    int width, height, channels;
    uint8_t *img_data =
        stbi_load(input_path.c_str(), &width, &height, &channels, 1);
    if (!img_data) {
        std::cerr << "Error: Failed to load image " << input_path << "\n";
        return 1;
    }

    std::vector<uint8_t> input_img(img_data, img_data + width * height);
    std::vector<uint8_t> output_img(width * height, 0);
    stbi_image_free(img_data);

    std::cout << "Loaded image: " << width << "x" << height << "\n";
    std::cout << "Mode: " << mode;
#ifdef HAS_OPENMP
    if (mode == "openmp") {
        // Let user override via OMP_NUM_THREADS env var OR the num_threads arg
        // (omp_set_num_threads must be called before the parallel region)
        omp_set_num_threads(num_threads);
        std::cout << " (threads=" << num_threads << ")";
    }
#endif
#ifdef HAS_PTHREADS
    if (mode == "pthreads")
        std::cout << " (threads=" << num_threads << ")";
#endif
    std::cout << "\n";

    // -----------------------------------------------------------------------
    // Optional reference image
    // -----------------------------------------------------------------------
    std::vector<uint8_t> reference_img;
    if (argc >= 6) {
        std::string ref_path = argv[5];
        int ref_w, ref_h, ref_ch;
        uint8_t *ref_data =
            stbi_load(ref_path.c_str(), &ref_w, &ref_h, &ref_ch, 1);
        if (ref_data) {
            if (ref_w == width && ref_h == height) {
                reference_img.assign(ref_data, ref_data + width * height);
                std::cout << "Reference image loaded for validation.\n";
            } else {
                std::cerr << "Warning: Reference image size mismatch. "
                             "Skipping validation.\n";
            }
            stbi_image_free(ref_data);
        } else {
            std::cerr << "Warning: Could not load reference image. "
                         "Skipping validation.\n";
        }
    }

    // -----------------------------------------------------------------------
    // Run selected implementation
    // -----------------------------------------------------------------------
    auto t0 = std::chrono::high_resolution_clock::now();

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
        std::cerr << "Unknown mode or not supported in this build: "
                  << mode << "\n";
        return 1;
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = t1 - t0;
    std::cout << "Execution time: " << elapsed.count() << " ms\n";

    // -----------------------------------------------------------------------
    // Save output
    // -----------------------------------------------------------------------
    if (!stbi_write_png(output_path.c_str(), width, height, 1,
                        output_img.data(), width)) {
        std::cerr << "Error: Failed to write output image\n";
        return 1;
    }
    std::cout << "Output saved to " << output_path << "\n";

    // -----------------------------------------------------------------------
    // Accuracy validation
    // -----------------------------------------------------------------------
    if (!reference_img.empty()) {
        double rmse = calculate_rmse(output_img, reference_img);
        double psnr = calculate_psnr(rmse);

        std::cout << "\n--- Accuracy vs. reference ---\n";
        std::cout << "  RMSE : " << rmse << "\n";

        if (psnr < 0.0)
            std::cout << "  PSNR : Infinity dB  (bit-perfect match)\n";
        else
            std::cout << "  PSNR : " << psnr << " dB\n";

        // Interpret result
        if (rmse == 0.0) {
            std::cout << "  Result: PERFECT – parallel output is identical "
                         "to the serial reference.\n";
        } else if (psnr >= 50.0) {
            std::cout << "  Result: EXCELLENT – negligible numerical "
                         "difference (floating-point rounding only).\n";
        } else if (psnr >= 40.0) {
            std::cout << "  Result: GOOD – minor differences, likely "
                         "acceptable.\n";
        } else {
            std::cout << "  Result: POOR – significant differences detected. "
                         "Check implementation.\n";
        }
    } else {
        std::cout << "\nTip: pass the serial output as a 5th argument to "
                     "measure RMSE/PSNR accuracy.\n";
        std::cout << "Example: " << argv[0]
                  << " input.png parallel_out.png pthreads 4 "
                     "serial_out.png\n";
    }

    return 0;
}