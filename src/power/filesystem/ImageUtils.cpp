#include "ImageUtils.hpp"

#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <cstring> // For std::memcpy

namespace {
// -------------------- Helper Functions --------------------

/**
 * @brief Flips the image vertically in-place.
 *
 * @param image     Pointer to the image pixel data.
 * @param width     The width of the image.
 * @param height    The height of the image.
 * @param channels  The number of color channels (e.g., 3 for RGB, 4 for RGBA).
 */
void flip_image_vertically(unsigned char* image, int width, int height, int channels) {
	int stride = width * channels;
	std::vector<unsigned char> row_buffer(stride);
	
	for (int y = 0; y < height / 2; ++y) {
		unsigned char* row_top = image + y * stride;
		unsigned char* row_bottom = image + (height - 1 - y) * stride;
		
		// Swap the rows
		std::memcpy(row_buffer.data(), row_top, stride);
		std::memcpy(row_top, row_bottom, stride);
		std::memcpy(row_bottom, row_buffer.data(), stride);
	}
}
} // unnamed namespace

// -------------------- JPEG Writing --------------------

// Callback function to handle JPEG data writing
void jpeg_write_callback(void* context, void* data, int size) {
	JpegWriteContext* ctx = static_cast<JpegWriteContext*>(context);
	uint8_t* bytes = static_cast<uint8_t*>(data);
	ctx->data.insert(ctx->data.end(), bytes, bytes + size);
}

/**
 * @brief Writes pixel data to a JPEG image stored in a memory buffer.
 *
 * @param pixels        The input pixel data (e.g., RGB or RGBA).
 * @param width         The width of the image.
 * @param height        The height of the image.
 * @param channels      The number of color channels (typically 3 for RGB or 4 for RGBA).
 * @param output_jpeg   The output vector where JPEG data will be stored.
 * @param quality       JPEG quality (1-100). Higher means better quality and larger size.
 */
void write_to_jpeg(const std::vector<uint8_t>& pixels,
				   int width,
				   int height,
				   int channels,
				   std::vector<uint8_t>& output_jpeg,
				   int quality) {
	JpegWriteContext ctx; // Initialize the context with an empty vector
	
	// Use stb_image_write's function to write JPEG to the callback
	if (!stbi_write_jpg_to_func(jpeg_write_callback, &ctx, width, height, channels, pixels.data(), quality)) {
		std::cerr << "Error writing JPEG to memory" << std::endl;
	} else {
		output_jpeg = std::move(ctx.data); // Move the data to the output vector
		std::cout << "JPEG written successfully to memory, size: " << output_jpeg.size() << " bytes" << std::endl;
	}
}

// -------------------- PNG Writing --------------------

// Callback function to handle PNG data writing
void png_write_callback(void* context, void* data, int size) {
	PngWriteContext* ctx = static_cast<PngWriteContext*>(context);
	uint8_t* bytes = static_cast<uint8_t*>(data);
	ctx->data.insert(ctx->data.end(), bytes, bytes + size);
}

/**
 * @brief Writes pixel data to a PNG image stored in a memory buffer.
 *
 * @param pixels          The input pixel data (e.g., RGB or RGBA).
 * @param width           The width of the image.
 * @param height          The height of the image.
 * @param channels        The number of color channels (typically 3 for RGB or 4 for RGBA).
 * @param output_png      The output vector where PNG data will be stored.
 * @param stride_in_bytes The number of bytes in a row of pixel data. If set to 0, it's calculated as width * channels.
 * @return true if the PNG was successfully written to the vector; false otherwise.
 */
bool write_to_png(const std::vector<uint8_t>& pixels,
				  int width,
				  int height,
				  int channels,
				  std::vector<uint8_t>& output_png,
				  int stride_in_bytes) {
	PngWriteContext ctx; // Initialize the context with an empty vector
	
	// If stride_in_bytes is not provided, calculate it
	if (stride_in_bytes == 0) {
		stride_in_bytes = width * channels;
	}
	
	// Create a copy of the pixel data to manipulate (flip vertically)
	std::vector<unsigned char> flipped_pixels = pixels;
	
	// Flip the image vertically
	flip_image_vertically(flipped_pixels.data(), width, height, channels);
	
	// Use stb_image_write's function to write PNG to the callback
	if (!stbi_write_png_to_func(png_write_callback, &ctx, width, height, channels, flipped_pixels.data(), stride_in_bytes)) {
		std::cerr << "Error writing PNG to memory" << std::endl;
		return false;
	}
	
	output_png = std::move(ctx.data); // Move the data to the output vector
	std::cout << "PNG written successfully to memory, size: " << output_png.size() << " bytes" << std::endl;
	return true;
}

// -------------------- PNG Writing to File --------------------

/**
 * @brief Writes pixel data to a PNG file with vertical flipping.
 *
 * @param imageData  The input image data in memory (e.g., downloaded from a URL).
 * @param filename   The filename where the PNG will be saved.
 * @return true if the image was successfully saved; false otherwise.
 */
bool write_compressed_png_to_file(const std::vector<uint8_t>& imageData, const std::string& filename) {
	// Decode image data from memory
	int width, height, channels;
	unsigned char* img = stbi_load_from_memory(imageData.data(), static_cast<int>(imageData.size()), &width, &height, &channels, 4);
	if (!img) {
		std::cerr << "Failed to decode image data for saving." << std::endl;
		return false;
	}
	
	// Flip the image vertically
	flip_image_vertically(img, width, height, 4);
	
	// Save the flipped image using stb_image_write with compression
	if (stbi_write_png(filename.c_str(), width, height, 4, img, width * 4)) {
		std::cout << "Image saved successfully to " << filename << std::endl;
	} else {
		std::cerr << "Failed to save image to " << filename << std::endl;
		stbi_image_free(img); // Free memory before returning
		return false;
	}
	
	// Free the image memory
	stbi_image_free(img);
	
	return true;
}

// -------------------- New Function: Reading PNG --------------------

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
							 int& channels) {
	// Load image data from file
	unsigned char* img = stbi_load(filename.c_str(), &width, &height, &channels, 4); // Force RGBA
	if (!img) {
		std::cerr << "Failed to load image from " << filename << std::endl;
		return false;
	}
	
	// Calculate the total number of pixels
	size_t total_pixels = static_cast<size_t>(width) * static_cast<size_t>(height) * 4; // RGBA
	
	// Copy pixel data into the vector
	pixels.assign(img, img + total_pixels);
	
	// Free the image memory
	stbi_image_free(img);
	
	std::cout << "Image loaded and processed successfully from " << filename << std::endl;
	return true;
}
