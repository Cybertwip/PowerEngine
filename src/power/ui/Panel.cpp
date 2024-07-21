#include "ui/Panel.hpp"

#include <GLFW/glfw3.h>

Panel::Panel(nanogui::Widget &parent, const std::string &title) : nanogui::Window(&parent, title) {}

bool Panel::mouse_drag_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button,
                             int modifiers) {
    // Disable dragging
    return false;
}

bool Panel::mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) {
    // Disable resizing
    if (down && (button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_RIGHT)) {
        return true;
    }
    return nanogui::Window::mouse_button_event(p, button, down, modifiers);
}
