#include "SharedSelfContainedMeshCanvas.hpp"

#include "filesystem/ImageUtils.hpp"

#include <nanogui/screen.h>

#if defined(NANOGUI_USE_METAL)
#include <nanogui/metal.h>
#include "MetalHelper.hpp"
#endif

SharedSelfContainedMeshCanvas::SharedSelfContainedMeshCanvas(nanogui::Widget& parent, nanogui::Screen& screen, AnimationTimeProvider& previewTimeProvider)
: SelfContainedMeshCanvas(parent, screen, previewTimeProvider) {
}

void SharedSelfContainedMeshCanvas::set_active_actor(std::shared_ptr<Actor> actor) {
	std::unique_lock<std::mutex> lock(mPreviewMutex);
	
	clear();

	
	if (actor != nullptr) {
		mSharedPreviewActor = actor;
		SelfContainedMeshCanvas::set_active_actor(*mSharedPreviewActor);
	} else {
		set_update(false);
		mSharedPreviewActor = nullptr;
	}
}

void SharedSelfContainedMeshCanvas::draw_contents() {
	std::unique_lock<std::mutex> lock(mPreviewMutex);

	SelfContainedMeshCanvas::draw_contents();
}

void SharedSelfContainedMeshCanvas::take_snapshot(std::function<void(std::vector<uint8_t>&)> onSnapshotTaken) {
	mSnapshotCallback = std::move(onSnapshotTaken);
}

void SharedSelfContainedMeshCanvas::process_events() {
	// schedule here
	if (mSnapshotCallback) {
		auto& scr = screen();
		
		void *texture =
		scr.metal_texture();
		
		
		float pixel_ratio = scr.pixel_ratio();
		
		nanogui::Vector2i fbsize = m_size;
		nanogui::Vector2i offset = absolute_position();
		fbsize = nanogui::Vector2i(nanogui::Vector2f(fbsize) * pixel_ratio);
		offset = nanogui::Vector2i(nanogui::Vector2f(offset) * pixel_ratio);
		
		std::vector<uint8_t> pixels (fbsize.x() * fbsize.y() * 4);
		
		MetalHelper::readPixelsFromMetal(scr.nswin(), texture, offset.x(), offset.y(), fbsize.x(), fbsize.y(), pixels);
		
		
		std::vector<uint8_t> png_data;
		
		// Now write the converted RGBA data to PNG
		write_to_jpeg(pixels, 512, 512, 4, png_data);
		
		mSnapshotCallback(png_data);
		mSnapshotCallback = nullptr;
	}
	
}
