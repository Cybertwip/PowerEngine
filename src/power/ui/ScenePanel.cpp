#include "ui/ScenePanel.hpp"

#include <nanogui/layout.h>
#include <nanogui/window.h>

ScenePanel::ScenePanel(nanogui::Widget *parent, RenderManager& renderManager) : nanogui::Widget(parent) {
    // Create a window
    nanogui::Window *window = new nanogui::Window(this, "Scene");

    // Set window position and layout
    window->set_position(nanogui::Vector2i(0, 0));
    window->set_layout(new nanogui::GroupLayout());

    // Create and configure a canvas
    mCanvas = new Canvas(window, renderManager);
    mCanvas->set_background_color({100, 100, 100, 255});
    mCanvas->set_fixed_size({900, 600});
}
