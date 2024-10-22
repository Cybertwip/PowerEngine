#pragma once

#include <vector>
#include <cstdint>
#include <iostream>
#include <string>

// Structure to hold JPEG data
struct JpegWriteContext {
	std::vector<uint8_t> data;
};

// Callback function to append JPEG data to the vector
void jpeg_write_callback(void* context, void* data, int size);

// Function to write pixel data to a JPEG image stored in a memory buffer
void write_to_jpeg(const std::vector<uint8_t>& pixels,
				   int width,
				   int height,
				   int channels,
				   std::vector<uint8_t>& output_jpeg,
				   int quality = 95);

// -------------------- New Functions for Writing to Files --------------------

// Function to write pixel data to a JPEG file on disk
bool write_compressed_png_to_file(const std::vector<uint8_t>& imageData, const std::string& filename);
