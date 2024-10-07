#include "SharedSelfContainedMeshCanvas.hpp"

#include "filesystem/ImageUtils.hpp"

#if defined(NANOGUI_USE_METAL)
#include <nanogui/metal.h>
#include "MetalHelper.hpp"
#endif

SharedSelfContainedMeshCanvas::SharedSelfContainedMeshCanvas(std::shared_ptr<Widget> parent)
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
		write_to_jpeg(pixels, 512, 512, 4, png_data);
		
		mSnapshotCallback(png_data);
		mSnapshotCallback = nullptr;
	}
	
}
