#include "ImageUtils.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image_write.h"


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
	
	// Calculate the stride (number of bytes per row)
	int stride_bytes = width * channels;
	
	// Use stb_image_write's function to write JPEG to the callback
	if (!stbi_write_jpg_to_func(jpeg_write_callback, &ctx, width, height, channels, pixels.data(), quality)) {
		std::cerr << "Error writing JPEG to memory" << std::endl;
	} else {
		output_jpeg = std::move(ctx.data); // Move the data to the output vector
		std::cout << "JPEG written successfully to memory, size: " << output_jpeg.size() << " bytes" << std::endl;
	}
}
