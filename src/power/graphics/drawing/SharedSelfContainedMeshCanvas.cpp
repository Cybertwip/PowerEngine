#include "SharedSelfContainedMeshCanvas.hpp"

#if defined(NANOGUI_USE_METAL)
#include <nanogui/metal.h>
#include "MetalHelper.hpp"
#endif

SharedSelfContainedMeshCanvas::SharedSelfContainedMeshCanvas(Widget* parent)
: SelfContainedMeshCanvas(parent) {

}

void SharedSelfContainedMeshCanvas::set_active_actor(std::shared_ptr<Actor> actor) {
	mSharedPreviewActor = actor;
	
	if (mSharedPreviewActor != nullptr) {
		SelfContainedMeshCanvas::set_active_actor(*mSharedPreviewActor);
	}
}

void SharedSelfContainedMeshCanvas::take_snapshot(std::function<void(std::vector<uint8_t>&)> onSnapshotTaken) {
	mSnapshotCallback = onSnapshotTaken;
	
	nanogui::Screen *scr = screen();
	if (scr == nullptr)
		throw std::runtime_error("Canvas::draw(): could not find parent screen!");
	
	auto drawable = nanogui::metal_window_next_drawable(scr->nswin());
	auto texture = nanogui::metal_drawable_texture(drawable);

	float pixel_ratio = scr->pixel_ratio();
	
	nanogui::Vector2i fbsize = m_size;
	nanogui::Vector2i offset = absolute_position();
	fbsize = nanogui::Vector2i(nanogui::Vector2f(fbsize) * pixel_ratio);
	offset = nanogui::Vector2i(nanogui::Vector2f(offset) * pixel_ratio);

	std::vector<uint8_t> pixels (fbsize.x() * fbsize.y() * 4);
	
	MetalHelper::readPixelsFromMetal(scr->nswin(), texture, offset.x(), offset.y(), fbsize.x(), fbsize.y(), pixels);
	
	mSnapshotCallback(pixels);
}