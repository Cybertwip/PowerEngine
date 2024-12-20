#include "ui/Panel.hpp"

#include <GLFW/glfw3.h>

Panel::Panel(nanogui::Widget& parent, const std::string &title) : nanogui::Window(parent, title) {
	
	set_fixed_size(parent.fixed_size());
}

bool Panel::mouse_drag_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button,
							 int modifiers) {
	// Disable dragging
	return false;
}

bool Panel::mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) {
	
	// event propagation depends on delegation
	return nanogui::Widget::mouse_button_event(p, button, down, modifiers);
}

bool Panel::scroll_event(const nanogui::Vector2i &p, const nanogui::Vector2f &rel) {
	
	// event propagation depends on delegation
	return nanogui::Widget::scroll_event(p, rel);
}
