#include "ui/ScenePanel.hpp"

#include <nanogui/layout.h>
#include <nanogui/window.h>

ScenePanel::ScenePanel(nanogui::Widget *parent) : nanogui::Window(parent, "Scene") {
    set_position(nanogui::Vector2i(0, 0));
    set_layout(new nanogui::GroupLayout());
}
