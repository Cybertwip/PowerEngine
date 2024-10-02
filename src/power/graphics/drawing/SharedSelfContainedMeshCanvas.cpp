#include "SharedSelfContainedMeshCanvas.hpp"

#if defined(NANOGUI_USE_METAL)
#include <nanogui/metal.h>
#include "MetalHelper.hpp"
#endif


#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image_write.h"

// Structure to hold PNG data
struct PngWriteContext {
	std::vector<uint8_t> data;
};

// Callback function to append data to the vector
static void png_write_callback(void* context, void* data, int size) {
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


SharedSelfContainedMeshCanvas::SharedSelfContainedMeshCanvas(Widget* parent)
: SelfContainedMeshCanvas(parent) {
}

void SharedSelfContainedMeshCanvas::set_active_actor(std::shared_ptr<Actor> actor) {
	clear();

	mSharedPreviewActor = actor;
	
	if (mSharedPreviewActor != nullptr) {
		SelfContainedMeshCanvas::set_active_actor(*mSharedPreviewActor);
	} else {
		set_update(false);
	}
}

void SharedSelfContainedMeshCanvas::draw_contents() {
	SelfContainedMeshCanvas::draw_contents();
}

void SharedSelfContainedMeshCanvas::take_snapshot(std::function<void(std::vector<uint8_t>&)> onSnapshotTaken) {
	mSnapshotCallback = std::move(onSnapshotTaken);
}

void SharedSelfContainedMeshCanvas::process_events() {
	
	// schedule here
	if (mSnapshotCallback) {
		nanogui::Screen *scr = screen();
		if (scr == nullptr)
			throw std::runtime_error("Canvas::draw(): could not find parent screen!");
		
		void *texture =
		scr->metal_texture();
		
		
		float pixel_ratio = scr->pixel_ratio();
		
		nanogui::Vector2i fbsize = m_size;
		nanogui::Vector2i offset = absolute_position();
		fbsize = nanogui::Vector2i(nanogui::Vector2f(fbsize) * pixel_ratio);
		offset = nanogui::Vector2i(nanogui::Vector2f(offset) * pixel_ratio);
		
		std::vector<uint8_t> pixels (fbsize.x() * fbsize.y() * 4);
		
		MetalHelper::readPixelsFromMetal(scr->nswin(), texture, offset.x(), offset.y(), fbsize.x(), fbsize.y(), pixels);
		
		
		std::vector<uint8_t> png_data;
		
		// Now write the converted RGBA data to PNG
		write_to_png(pixels, 512, 512, 4, png_data);
		
		mSnapshotCallback(png_data);
		mSnapshotCallback = nullptr;
	}
	
}
