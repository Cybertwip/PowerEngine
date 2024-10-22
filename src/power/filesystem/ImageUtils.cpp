#include "ImageUtils.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

namespace {
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
}

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
