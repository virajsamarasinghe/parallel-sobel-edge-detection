#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../include/stb/stb_image_write.h"
#include <vector>
#include <cmath>
#include <iostream>

int main() {
    int width = 3840;
    int height = 2160;
    std::vector<uint8_t> data(width * height);

    std::cout << "Generating 4K synthetic image (" << width << "x" << height << ")..." << std::endl;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Create nice grid lines and a circular gradient pattern to generate edges
            if ((x / 120) % 2 == 0 || (y / 120) % 2 == 0) {
                data[y * width + x] = 255;
            } else {
                float dx = x - width / 2;
                float dy = y - height / 2;
                float dist = std::sqrt(dx * dx + dy * dy);
                data[y * width + x] = static_cast<uint8_t>(128 + 127 * std::sin(dist / 60.0f));
            }
        }
    }

    std::string output_path = "data/image_4k.png";
    if (stbi_write_png(output_path.c_str(), width, height, 1, data.data(), width)) {
        std::cout << "Successfully saved 4K test image to " << output_path << std::endl;
        return 0;
    } else {
        std::cerr << "Error: Failed to save 4K image." << std::endl;
        return 1;
    }
}
