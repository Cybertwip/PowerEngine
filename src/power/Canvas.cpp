#include "Canvas.hpp"

#include "actors/ActorManager.hpp"
#include "filesystem/ImageUtils.hpp"
#include "graphics/drawing/Drawable.hpp"

#include <nanogui/renderpass.h>
#include <nanogui/screen.h>

#if defined(NANOGUI_USE_METAL)
#include <nanogui/metal.h>
#include "MetalHelper.hpp"
#endif

#include <GLFW/glfw3.h>

#include <iostream>

Canvas::Canvas(nanogui::Widget& parent, nanogui::Screen& screen, nanogui::Color backgroundColor) : nanogui::Canvas(parent, screen, 1, true, true), mBackgroundColor(backgroundColor) {
	
	set_background_color(mBackgroundColor);
	set_fixed_size(parent.fixed_size());

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
		write_to_jpeg(pixels, fbsize.x(), fbsize.y(), 4, png_data);
		
		mSnapshotCallback(png_data);
		mSnapshotCallback = nullptr;
	}
	
}

bool Canvas::mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) {
	if (button == GLFW_MOUSE_BUTTON_1 && down) {
		request_focus();
	}
	
	// Delegate event
	return nanogui::Canvas::mouse_button_event(p, button, down, modifiers);
}

