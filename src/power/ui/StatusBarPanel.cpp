#include "ui/StatusBarPanel.hpp"

#include <nanogui/button.h>
#include <nanogui/icons.h>
#include <nanogui/layout.h>
#include <nanogui/toolbutton.h>
#include <nanogui/window.h>

StatusBarPanel::StatusBarPanel(nanogui::Widget &parent) : Panel(parent, "") {
    set_position(nanogui::Vector2i(0, 0));
    set_layout(new nanogui::GroupLayout());

    Widget *statusBar = new Widget(this);
    statusBar->set_layout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal,
                                                 nanogui::Alignment::Minimum, 0, 0));

    nanogui::Button *resourcesButton = new nanogui::ToolButton(statusBar, FA_FOLDER);
    resourcesButton->set_callback([this]() {
        // mCanvas->set_background_color(nanogui::Vector4i(rand() % 256, rand() % 256, rand() %
        // 256, 255));
    });
}
