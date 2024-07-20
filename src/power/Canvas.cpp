#include "Canvas.hpp"

#include "graphics/drawing/Drawable.hpp"

Canvas::Canvas(Widget *parent) : nanogui::Canvas(parent, 1) {
}

void Canvas::draw_contents() {
	for (auto drawable : drawables) {
		drawable.get().draw_content(*this);
	}
	drawables.clear();
}

void Canvas::add_drawable(std::reference_wrapper<Drawable> drawable) {
	drawables.push_back(drawable);
}
