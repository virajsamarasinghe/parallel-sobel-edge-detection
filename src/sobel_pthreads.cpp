
// sobel_pthreads.cpp  –  Pthreads parallel Sobel edge detection




#include "../include/sobel.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <pthread.h>
#include <vector>


// Per-thread argument bundle

struct ThreadData {
    const std::vector<uint8_t> *input;
    std::vector<uint8_t>       *output;
    int width;
    int height;
    int start_row;   // inclusive
    int end_row;     // exclusive
};


// Worker function executed by each thread

static void *sobel_thread(void *arg) {
    ThreadData *d = static_cast<ThreadData *>(arg);

    const int Gx[3][3] = {{-1, 0, 1},
                           {-2, 0, 2},
                           {-1, 0, 1}};

    const int Gy[3][3] = {{ 1,  2,  1},
                           { 0,  0,  0},
                           {-1, -2, -1}};

    // Clamp to the valid inner region [1, height-1)
    int y_start = std::max(d->start_row, 1);
    int y_end   = std::min(d->end_row,   d->height - 1);

    for (int y = y_start; y < y_end; ++y) {
        for (int x = 1; x < d->width - 1; ++x) {
            int sumX = 0;
            int sumY = 0;

            for (int j = -1; j <= 1; ++j) {
                for (int i = -1; i <= 1; ++i) {
                    int pixel_val = static_cast<int>(
                        (*d->input)[(y + j) * d->width + (x + i)]);
                    sumX += pixel_val * Gx[j + 1][i + 1];
                    sumY += pixel_val * Gy[j + 1][i + 1];
                }
            }

            int magnitude = static_cast<int>(std::sqrt(
                static_cast<double>(sumX * sumX + sumY * sumY)));

            (*d->output)[y * d->width + x] =
                static_cast<uint8_t>(std::min(255, magnitude));
        }
    }

    return nullptr;
}

// Public API

void sobel_pthreads(const std::vector<uint8_t> &input,
                    std::vector<uint8_t>       &output,
                    int width, int height,
                    int num_threads) {

    if (width < 3 || height < 3) return;
    if (num_threads < 1) num_threads = 1;

    // Zero border pixels in the main thread
    for (int x = 0; x < width; ++x) {
        output[0 * width + x]            = 0;
        output[(height - 1) * width + x] = 0;
    }
    for (int y = 0; y < height; ++y) {
        output[y * width + 0]           = 0;
        output[y * width + (width - 1)] = 0;
    }

    // Allocate thread handles and argument bundles on the heap (not VLAs)
    std::vector<pthread_t>  threads(num_threads);
    std::vector<ThreadData> thread_data(num_threads);

    int rows_per_thread = height / num_threads;

    for (int i = 0; i < num_threads; ++i) {
        thread_data[i].input     = &input;
        thread_data[i].output    = &output;
        thread_data[i].width     = width;
        thread_data[i].height    = height;
        thread_data[i].start_row = i * rows_per_thread;
        // Last thread gets any leftover rows
        thread_data[i].end_row   = (i == num_threads - 1)
                                       ? height
                                       : (i + 1) * rows_per_thread;

        int rc = pthread_create(&threads[i], nullptr,
                                sobel_thread, &thread_data[i]);
        if (rc != 0) {
            std::fprintf(stderr,
                "sobel_pthreads: pthread_create failed for thread %d (rc=%d)\n",
                i, rc);
        }
    }

    for (int i = 0; i < num_threads; ++i)
        pthread_join(threads[i], nullptr);
}