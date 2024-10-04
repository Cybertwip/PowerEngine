#pragma once

// Structure to hold PNG data
struct JpegWriteContext {
	std::vector<uint8_t> data;
};

// Callback function to append data to the vector
void jpeg_write_callback(void* context, void* data, int size);

void write_to_jpeg(const std::vector<uint8_t>& pixels,
			  int width,
			  int height,
			  int channels,
			  std::vector<uint8_t>& output_jpeg,
			  int quality = 95);
