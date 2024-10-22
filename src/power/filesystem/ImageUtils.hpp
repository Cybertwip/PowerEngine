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

// Structure to hold PNG data
struct PngWriteContext {
	std::vector<uint8_t> data;
};

// Callback function to append PNG data to the vector
void png_write_callback(void* context, void* data, int size);

// Function to write pixel data to a PNG image stored in a memory buffer
bool write_to_png(const std::vector<uint8_t>& pixels,
				  int width,
				  int height,
				  int channels,
				  std::vector<uint8_t>& output_png,
				  int stride_in_bytes = 0);

// Function to write pixel data to a PNG file with vertical flipping
bool write_compressed_png_to_file(const std::vector<uint8_t>& imageData, const std::string& filename);

// -------------------- New Functions for Reading --------------------

/**
 * @brief Reads a PNG file, decodes it into pixel data, flips it vertically, and stores it in a vector.
 *
 * @param filename   The path to the PNG file to be read.
 * @param pixels     The output vector where pixel data will be stored.
 * @param width      The output width of the image in pixels.
 * @param height     The output height of the image in pixels.
 * @param channels   The output number of color channels (e.g., 3 for RGB, 4 for RGBA).
 * @return true if the image was successfully read and processed; false otherwise.
 */
bool read_png_file_to_vector(const std::string& filename,
							 std::vector<uint8_t>& pixels,
							 int& width,
							 int& height,
							 int& channels);
