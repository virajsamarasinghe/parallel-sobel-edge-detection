# Phase 1 Code Structure and Explanation

This document provides a breakdown of how the Phase 1 Serial Baseline for the Sobel Edge Detection project is structured and what each part of the code does.

## File Structure

The codebase is organized into standard C++ directories:
*   `include/`: Contains the header files (`.h`) which declare our functions so they can be shared across multiple files, and also the STB image libraries used to read/write images without needing complex software like OpenCV to be installed.
*   `src/`: Contains the implementation files (`.cpp`) where the actual code logic lives.
*   `data/`: Contains our test images (e.g., `lenna.png`) and the generated output image.

## Detailed Breakdown

### 1. Image Loading and Saving (`src/main.cpp`)
To read an image pixel-by-pixel, we use a very popular library called `stb_image`.
*   We use `stbi_load(input_path.c_str(), &width, &height, &channels, 1)` to load the image. The `1` at the end forces the image to be loaded as grayscale. This is crucial because edge detection usually happens on the intensity (brightness) of the image, rather than the separate Red, Green, and Blue colors.
*   The loaded data comes as a raw array. We immediately convert this raw data into a modern C++ `std::vector<uint8_t>`. A `uint8_t` is exactly 1 byte (0-255), representing a single pixel's brightness.
*   At the end of processing, we use `stbi_write_png` to save our modified vector back into a standard PNG file.

### 2. The Core Algorithm (`src/sobel.cpp`)
This is where the math happens. The Sobel operator looks for strong transitions from light to dark (or dark to light) which represent edges.

**The Masks:**
We define two tiny 3x3 matrices (called "kernels" or "masks"):
*   `Gx` looks for vertical edges (changes from left to right).
*   `Gy` looks for horizontal edges (changes from top to bottom).

**The Convolution Loop:**
*   We loop through every single pixel in the image using nested `for` loops (one for the Y coordinate, one for the X coordinate). *Note: We skip the very outer 1-pixel border of the image to avoid going "out of bounds" when applying the 3x3 mask.*
*   For each pixel, we look at its 8 neighbors (the 3x3 area around it). We multiply the brightness of those neighbors by the values in the `Gx` mask and add them all up to get `sumX`. We do the same for the `Gy` mask to get `sumY`.
*   If `sumX` is high, there is a strong vertical edge. If `sumY` is high, there is a strong horizontal edge.

**Combining the Results (Magnitude):**
*   To find the true "strength" of the edge regardless of whether it's horizontal or vertical, we use the Pythagorean theorem: `Magnitude = sqrt(sumX^2 + sumY^2)`.
*   This magnitude might be larger than 255. Since a pixel can only hold a value from 0 to 255, we use `std::min(255, magnitude)` to "clamp" any overflow, ensuring the edge gets drawn as bright white (255) rather than causing an error.

### 3. Measuring Performance (`src/main.cpp`)
Around the call to `sobel_serial`, we use `std::chrono::high_resolution_clock::now()`. This records the exact time before the convolution starts, and the exact time after it finishes. Subtracting these gives us the execution time in milliseconds. When we move to Phase 2 (Shared Memory), we will compare that new execution time against this baseline to calculate our speedup!

### 4. Verification Metrics (`src/utils.cpp`)
These functions ensure that as we parallelize the code in later phases, we don't accidentally break the math.
*   **RMSE (Root Mean Square Error):** Measures how much, on average, the pixels of our fast parallel output differ from the pixels of our slow-but-correct serial output. A value of `0` means they are perfectly identical.
*   **PSNR (Peak Signal-to-Noise Ratio):** This translates RMSE into a logarithmic "Decibel" score (like sound volumes). A higher DB score is better. If the images match exactly, the PSNR technically approaches Infinity.

By passing an optional third argument to the executable (`./build/sobel_serial data/lenna.png data/output.png data/reference.png`), the application will load that third image and calculate these metrics for you automatically.
