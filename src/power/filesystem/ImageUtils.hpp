#pragma once

// Structure to hold PNG data
struct PngWriteContext {
	std::vector<uint8_t> data;
};

// Callback function to append data to the vector
void png_write_callback(void* context, void* data, int size);

static void write_to_png(const std::vector<uint8_t>& pixels, int width, int height, int channels, std::vector<uint8_t>& output_png);
