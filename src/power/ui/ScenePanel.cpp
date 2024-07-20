#include "ui/ScenePanel.hpp"

#include <nanogui/button.h>
#include <nanogui/icons.h>
#include <nanogui/layout.h>
#include <nanogui/toolbutton.h>
#include <nanogui/window.h>

ScenePanel::ScenePanel(nanogui::Widget *parent) : nanogui::Window(parent, "Scene") {
    set_position(nanogui::Vector2i(0, 0));
    set_layout(new nanogui::GroupLayout());

    Widget *tools = new Widget(this);
    tools->set_layout(
        new nanogui::BoxLayout(nanogui::Orientation::Horizontal, nanogui::Alignment::Middle, 0, 5));

    nanogui::Button *b0 = new nanogui::ToolButton(tools, FA_PLAY);
    b0->set_callback([this]() {
        // mCanvas->set_background_color(nanogui::Vector4i(rand() % 256, rand() % 256, rand() %
        // 256, 255));
    });

    nanogui::Button *b1 = new nanogui::Button(tools, "Random Rotation");
    b1->set_callback([this]() {
        // m_canvas->set_rotation((float) M_PI * rand() / (float) RAND_MAX);
    });

    /* Create an empty panel with a horizontal layout */
    Widget *panel = new Widget(this);
    panel->set_layout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal,
                                             nanogui::Alignment::Middle, 0, 20));
}
