#include "ui/ScenePanel.hpp"

#include <nanogui/button.h>
#include <nanogui/icons.h>
#include <nanogui/layout.h>
#include <nanogui/toolbutton.h>
#include <nanogui/window.h>

ScenePanel::ScenePanel(nanogui::Widget &parent) : Panel(parent, "Scene") {
    set_position(nanogui::Vector2i(0, 0));
    set_layout(new nanogui::GridLayout(nanogui::Orientation::Vertical, 1, nanogui::Alignment::Fill));
}
