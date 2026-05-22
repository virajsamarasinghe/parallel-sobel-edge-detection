
#include <mpi.h>
#include <omp.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
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

#include "../include/utils.h"


//  Applies the Sobel filter to a local chunk of rows using OpenMP threads.
//
//  The input buffer includes ghost rows above/below so the 3x3 kernel can be
//  evaluated at chunk boundaries. Only rows [y_start, y_end) are written.
//
//  This function performs NO MPI calls, so it is safe to run multithreaded
//  under the MPI_THREAD_FUNNELED model.

static void sobel_local(const uint8_t* input, uint8_t* output,
                        int width, int total_rows,
                        int y_start, int y_end)
{
    (void)total_rows;  // documented for clarity; bounds come from y_start/y_end

    const int Gx[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    const int Gy[3][3] = {{ 1, 2, 1}, { 0, 0, 0}, {-1,-2,-1}};

    // OpenMP shared-memory parallelism: distribute the rows of this rank's
    // block across the thread team. Each row is independent, so a static
    // schedule gives balanced, cache-friendly work distribution.
    #pragma omp parallel for schedule(static)
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
    // -- Step 0: Initialise MPI with thread support --
    //
    //  MPI_THREAD_FUNNELED is requested because each process is multithreaded
    //  (OpenMP) but every MPI call is issued only by the main thread.
    int provided = MPI_THREAD_SINGLE;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);

    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    if (rank == 0 && provided < MPI_THREAD_FUNNELED) {
        std::cerr << "Warning: MPI implementation does not provide "
                     "MPI_THREAD_FUNNELED (got level " << provided << ").\n";
    }

    if (argc < 3) {
        if (rank == 0)
            std::cerr << "Usage: mpirun -np <N> " << argv[0]
                      << " <input_image> <output_image> [reference_image]\n"
                      << "       (set OMP_NUM_THREADS to choose threads per rank)\n";
        MPI_Finalize();
        return 1;
    }

    std::string input_path  = argv[1];
    std::string output_path = argv[2];
    std::string ref_path    = (argc >= 4) ? argv[3] : "";

    int threads_per_rank = omp_get_max_threads();

    int width = 0, height = 0;
    std::vector<uint8_t> full_input;
    std::vector<uint8_t> full_output;

    // -- Step 1: Master (Rank 0) loads the image --
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
                  << " | MPI processes: " << num_procs
                  << " | OpenMP threads/rank: " << threads_per_rank
                  << " | Total parallel units: "
                  << num_procs * threads_per_rank << std::endl;
    }

    // -- Step 2: Broadcast image dimensions to all ranks --
    MPI_Bcast(&width,  1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&height, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // -- Step 3: Compute row distribution (domain decomposition) --
    //
    //  Rows are spread as evenly as possible; the first `remainder` ranks
    //  each take one extra row so no rows are lost when height % P != 0.
    int base_rows = height / num_procs;
    int remainder = height % num_procs;

    std::vector<int> row_counts(num_procs), row_offsets(num_procs);
    std::vector<int> sendcounts(num_procs), displs(num_procs);

    for (int i = 0; i < num_procs; ++i) {
        row_counts[i]  = base_rows + (i < remainder ? 1 : 0);
        row_offsets[i] = (i == 0) ? 0 : row_offsets[i - 1] + row_counts[i - 1];
        sendcounts[i]  = row_counts[i] * width;
        displs[i]      = row_offsets[i] * width;
    }

    int my_rows = row_counts[rank];
    std::vector<uint8_t> local_input(my_rows * width);

    // -- Synchronise and start timing --
    MPI_Barrier(MPI_COMM_WORLD);
    double t_start = MPI_Wtime();

    // -- Step 4: Scatter image rows from master to all workers --
    MPI_Scatterv(rank == 0 ? full_input.data() : nullptr,
                 sendcounts.data(), displs.data(), MPI_UNSIGNED_CHAR,
                 local_input.data(), my_rows * width, MPI_UNSIGNED_CHAR,
                 0, MPI_COMM_WORLD);

    // -- Step 5: Ghost-cell (halo) exchange via point-to-point messages --
    //
    //  Each rank needs 1 row from its top neighbour and 1 row from its bottom
    //  neighbour to evaluate the 3x3 kernel at the edges of its block.
    //  MPI_Sendrecv pairs a send and a receive in one deadlock-free call.
    bool has_top    = (rank > 0);
    bool has_bottom = (rank < num_procs - 1);

    std::vector<uint8_t> ghost_top(width, 0);
    std::vector<uint8_t> ghost_bottom(width, 0);

    // Top neighbour (rank - 1): send my first row up, receive their last row.
    if (has_top) {
        MPI_Sendrecv(local_input.data(), width, MPI_UNSIGNED_CHAR,
                     rank - 1, 0,
                     ghost_top.data(), width, MPI_UNSIGNED_CHAR,
                     rank - 1, 1,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    // Bottom neighbour (rank + 1): send my last row down, receive their first.
    if (has_bottom) {
        MPI_Sendrecv(local_input.data() + (my_rows - 1) * width,
                     width, MPI_UNSIGNED_CHAR, rank + 1, 1,
                     ghost_bottom.data(), width, MPI_UNSIGNED_CHAR,
                     rank + 1, 0,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    // -- Step 6: Build the extended buffer with ghost rows --
    //
    //  Layout: [ghost_top?] [my local rows] [ghost_bottom?]
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

    // -- Step 7: Apply Sobel convolution (OpenMP-parallel within this rank) --
    //
    //  In extended coordinates, computable rows are [1, ext_rows - 1) because
    //  the kernel needs y-1 and y+1. sobel_local() spreads these rows across
    //  the OpenMP thread team.
    double c_start = MPI_Wtime();
    sobel_local(ext_input.data(), ext_output.data(),
                width, ext_rows, 1, ext_rows - 1);
    double c_end = MPI_Wtime();

    // Extract this rank's output rows, dropping the ghost padding.
    std::vector<uint8_t> local_output(my_rows * width, 0);
    std::copy(ext_output.begin() + top_pad * width,
              ext_output.begin() + (top_pad + my_rows) * width,
              local_output.begin());

    // -- Step 8: Gather results back at the master --
    MPI_Gatherv(local_output.data(), my_rows * width, MPI_UNSIGNED_CHAR,
                rank == 0 ? full_output.data() : nullptr,
                sendcounts.data(), displs.data(), MPI_UNSIGNED_CHAR,
                0, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);
    double t_end = MPI_Wtime();

    // Report the slowest rank's compute time (the true parallel bottleneck).
    double my_compute_ms = (c_end - c_start) * 1000.0;
    double max_compute_ms = 0.0;
    MPI_Reduce(&my_compute_ms, &max_compute_ms, 1, MPI_DOUBLE, MPI_MAX,
               0, MPI_COMM_WORLD);

    // -- Step 9: Master saves output and runs verification --
    if (rank == 0) {
        double elapsed_ms = (t_end - t_start) * 1000.0;
        std::cout << "Hybrid Execution time: " << elapsed_ms << " ms"
                  << " (compute phase: " << max_compute_ms << " ms)"
                  << std::endl;

        if (!stbi_write_png(output_path.c_str(), width, height, 1,
                            full_output.data(), width)) {
            std::cerr << "Error: Failed to write output image\n";
        } else {
            std::cout << "Output saved to " << output_path << std::endl;
        }

        // Compare against a reference image (e.g. the serial output).
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
