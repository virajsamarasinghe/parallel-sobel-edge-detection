#include <mpi.h>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>

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

#include "../include/utils.h"

/**
 * Applies the Sobel filter on a local chunk of rows.
 * The input buffer includes ghost rows above/below for boundary computation.
 * Only rows [y_start, y_end) are computed.
 */
static void sobel_local(const uint8_t* input, uint8_t* output,
                        int width, int total_rows,
                        int y_start, int y_end)
{
    const int Gx[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    const int Gy[3][3] = {{ 1, 2, 1}, { 0, 0, 0}, {-1,-2,-1}};

    for (int y = y_start; y < y_end; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            int sumX = 0, sumY = 0;

            for (int j = -1; j <= 1; ++j) {
                for (int i = -1; i <= 1; ++i) {
                    int pixel = input[(y + j) * width + (x + i)];
                    sumX += pixel * Gx[j + 1][i + 1];
                    sumY += pixel * Gy[j + 1][i + 1];
                }
            }

            int mag = static_cast<int>(std::sqrt(sumX * sumX + sumY * sumY));
            output[y * width + x] = static_cast<uint8_t>(std::min(255, mag));
        }
    }
}

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);

    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    if (argc < 3) {
        if (rank == 0)
            std::cerr << "Usage: mpirun -np <N> " << argv[0]
                      << " <input_image> <output_image> [reference_image]\n";
        MPI_Finalize();
        return 1;
    }

    std::string input_path  = argv[1];
    std::string output_path = argv[2];
    std::string ref_path    = (argc >= 4) ? argv[3] : "";

    int width = 0, height = 0;
    std::vector<uint8_t> full_input;
    std::vector<uint8_t> full_output;

    // ── Step 1: Master (Rank 0) loads the image ──
    if (rank == 0) {
        int channels;
        uint8_t* img_data = stbi_load(input_path.c_str(), &width, &height,
                                      &channels, 1);
        if (!img_data) {
            std::cerr << "Error: Failed to load image " << input_path << "\n";
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        full_input.assign(img_data, img_data + width * height);
        full_output.resize(width * height, 0);
        stbi_image_free(img_data);

        std::cout << "Loaded image: " << width << "x" << height
                  << " | Processes: " << num_procs << std::endl;
    }

    // ── Step 2: Broadcast image dimensions to all ranks ──
    MPI_Bcast(&width,  1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&height, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // ── Step 3: Compute row distribution (domain decomposition) ──
    int base_rows = height / num_procs;
    int remainder = height % num_procs;

    std::vector<int> row_counts(num_procs), row_offsets(num_procs);
    std::vector<int> sendcounts(num_procs), displs(num_procs);

    for (int i = 0; i < num_procs; ++i) {
        row_counts[i]  = base_rows + (i < remainder ? 1 : 0);
        row_offsets[i]  = (i == 0) ? 0 : row_offsets[i - 1] + row_counts[i - 1];
        sendcounts[i]  = row_counts[i] * width;
        displs[i]      = row_offsets[i] * width;
    }

    int my_rows = row_counts[rank];
    std::vector<uint8_t> local_input(my_rows * width);

    // ── Synchronize and start timing ──
    MPI_Barrier(MPI_COMM_WORLD);
    double t_start = MPI_Wtime();

    // ── Step 4: Scatter image rows from master to all workers ──
    MPI_Scatterv(rank == 0 ? full_input.data() : nullptr,
                 sendcounts.data(), displs.data(), MPI_UNSIGNED_CHAR,
                 local_input.data(), my_rows * width, MPI_UNSIGNED_CHAR,
                 0, MPI_COMM_WORLD);

    // ── Step 5: Ghost-cell exchange using point-to-point communication ──
    //
    //  Each process needs 1 row from its top neighbor and 1 row from its
    //  bottom neighbor to compute the Sobel kernel at chunk boundaries.
    //
    //  Uses MPI_Sendrecv to avoid deadlock.
    //
    bool has_top    = (rank > 0);
    bool has_bottom = (rank < num_procs - 1);

    std::vector<uint8_t> ghost_top(width, 0);
    std::vector<uint8_t> ghost_bottom(width, 0);

    // Exchange with top neighbor (rank − 1):
    //   Send my first row UP,  receive their last row as my ghost_top
    if (has_top) {
        MPI_Sendrecv(local_input.data(), width, MPI_UNSIGNED_CHAR,
                     rank - 1, 0,
                     ghost_top.data(),   width, MPI_UNSIGNED_CHAR,
                     rank - 1, 1,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    // Exchange with bottom neighbor (rank + 1):
    //   Send my last row DOWN, receive their first row as my ghost_bottom
    if (has_bottom) {
        MPI_Sendrecv(local_input.data() + (my_rows - 1) * width,
                     width, MPI_UNSIGNED_CHAR, rank + 1, 1,
                     ghost_bottom.data(), width, MPI_UNSIGNED_CHAR,
                     rank + 1, 0,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    // ── Step 6: Build extended buffer with ghost rows ──
    //
    //  Layout: [ghost_top?] [local rows] [ghost_bottom?]
    //
    int top_pad  = has_top    ? 1 : 0;
    int bot_pad  = has_bottom ? 1 : 0;
    int ext_rows = top_pad + my_rows + bot_pad;

    std::vector<uint8_t> ext_input(ext_rows * width, 0);
    std::vector<uint8_t> ext_output(ext_rows * width, 0);

    if (has_top)
        std::copy(ghost_top.begin(), ghost_top.end(), ext_input.begin());

    std::copy(local_input.begin(), local_input.end(),
              ext_input.begin() + top_pad * width);

    if (has_bottom)
        std::copy(ghost_bottom.begin(), ghost_bottom.end(),
                  ext_input.begin() + (top_pad + my_rows) * width);

    // ── Step 7: Apply Sobel convolution on computable rows ──
    //
    //  In extended coordinates, computable rows are [1, ext_rows − 1)
    //  because the kernel needs access to y−1 and y+1.
    //
    sobel_local(ext_input.data(), ext_output.data(),
                width, ext_rows, 1, ext_rows - 1);

    // Extract this rank's output rows (skip ghost padding)
    std::vector<uint8_t> local_output(my_rows * width, 0);
    std::copy(ext_output.begin() + top_pad * width,
              ext_output.begin() + (top_pad + my_rows) * width,
              local_output.begin());

    // ── Step 8: Gather results at master ──
    MPI_Gatherv(local_output.data(), my_rows * width, MPI_UNSIGNED_CHAR,
                rank == 0 ? full_output.data() : nullptr,
                sendcounts.data(), displs.data(), MPI_UNSIGNED_CHAR,
                0, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);
    double t_end = MPI_Wtime();

    // ── Step 9: Master saves output and runs verification ──
    if (rank == 0) {
        double elapsed_ms = (t_end - t_start) * 1000.0;
        std::cout << "MPI Execution time: " << elapsed_ms << " ms" << std::endl;

        if (!stbi_write_png(output_path.c_str(), width, height, 1,
                            full_output.data(), width)) {
            std::cerr << "Error: Failed to write output image\n";
        } else {
            std::cout << "Output saved to " << output_path << std::endl;
        }

        // Compare against reference image (serial output) if provided
        if (!ref_path.empty()) {
            int rw, rh, rc;
            uint8_t* ref_data = stbi_load(ref_path.c_str(), &rw, &rh, &rc, 1);
            if (ref_data && rw == width && rh == height) {
                std::vector<uint8_t> ref_img(ref_data, ref_data + rw * rh);
                stbi_image_free(ref_data);

                double rmse = calculate_rmse(full_output, ref_img);
                double psnr = calculate_psnr(rmse);
                std::cout << "RMSE vs reference: " << rmse << std::endl;
                if (psnr < 0)
                    std::cout << "PSNR: Infinity (perfect match)" << std::endl;
                else
                    std::cout << "PSNR: " << psnr << " dB" << std::endl;
            } else {
                if (ref_data) stbi_image_free(ref_data);
                std::cerr << "Warning: Could not load or match reference image\n";
            }
        }
    }

    MPI_Finalize();
    return 0;
}
