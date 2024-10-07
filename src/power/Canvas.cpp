#include "Canvas.hpp"

#include "actors/ActorManager.hpp"
#include "filesystem/ImageUtils.hpp"
#include "graphics/drawing/Drawable.hpp"

#include <nanogui/renderpass.h>

#if defined(NANOGUI_USE_METAL)
#include <nanogui/metal.h>
#include "MetalHelper.hpp"
#endif


#include <iostream>

Canvas::Canvas(std::shared_ptr<Widget> parent, nanogui::Color backgroundColor) : nanogui::Canvas(parent, 1, true, true) {
    set_background_color(backgroundColor);
    set_fixed_size(parent->fixed_size());
}

void Canvas::draw_contents() {
    for (auto& callback : mDrawCallbacks) {
        callback();
    }
}

void Canvas::register_draw_callback(std::function<void()> callback) {
    mDrawCallbacks.push_back(callback);
}

void Canvas::take_snapshot(std::function<void(std::vector<uint8_t>&)> onSnapshotTaken) {
	mSnapshotCallback = std::move(onSnapshotTaken);
}

void Canvas::process_events() {
	
	// schedule here
	if (mSnapshotCallback) {
		auto scr = screen();
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
		write_to_jpeg(pixels, fbsize.x(), fbsize.y(), 4, png_data);
		
		mSnapshotCallback(png_data);
		mSnapshotCallback = nullptr;
	}
	
}
