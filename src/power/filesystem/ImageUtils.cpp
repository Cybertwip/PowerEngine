#include "ImageUtils.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image_write.h"

void png_write_callback(void* context, void* data, int size) {
	PngWriteContext* ctx = static_cast<PngWriteContext*>(context);
	uint8_t* bytes = static_cast<uint8_t*>(data);
	ctx->data.insert(ctx->data.end(), bytes, bytes + size);
}
// Function to write PNG data to a vector using stb_image_write
void write_to_png(const std::vector<uint8_t>& pixels, int width, int height, int channels, std::vector<uint8_t>& output_png) {
	PngWriteContext ctx; // Initialize the context with an empty vector
	
	// Calculate the stride (number of bytes per row)
	int stride_bytes = width * channels;
	
	// Use stb_image_write's function to write PNG to the callback
	if (!stbi_write_png_to_func(png_write_callback, &ctx, width, height, channels, pixels.data(), stride_bytes)) {
		std::cerr << "Error writing PNG to memory" << std::endl;
	} else {
		output_png = std::move(ctx.data); // Move the data to the output vector
		std::cout << "PNG written successfully to memory, size: " << output_png.size() << " bytes" << std::endl;
	}
}

