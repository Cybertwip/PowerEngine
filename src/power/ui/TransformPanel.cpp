#include "ui/TransformPanel.hpp"

#include <nanogui/button.h>
#include <nanogui/icons.h>
#include <nanogui/textbox.h>
#include <nanogui/label.h>
#include <nanogui/layout.h>
#include <nanogui/window.h>

TransformPanel::TransformPanel(nanogui::Widget *parent) : nanogui::Window(parent, "Transform") {
    set_position(nanogui::Vector2i(0, 0));
    set_layout(new nanogui::GroupLayout());

    // Translation section
    new nanogui::Label(this, "Translation", "sans-bold");

    Widget *translatePanel = new Widget(this);
    translatePanel->set_layout(new nanogui::GridLayout(nanogui::Orientation::Vertical, 2,
                                                       nanogui::Alignment::Middle, 15, 5));
    
    new nanogui::Label(translatePanel, "X:", "sans-bold");
    auto x_translate = new nanogui::IntBox<int>(translatePanel);
    x_translate->set_editable(true);
    x_translate->set_fixed_size(nanogui::Vector2i(100, 20));
    x_translate->set_value(0);
    x_translate->set_default_value("0");
    x_translate->set_font_size(16);
    x_translate->set_format("[-]?[0-9]*");
    x_translate->set_spinnable(true);
    x_translate->set_value_increment(1);

    new nanogui::Label(translatePanel, "Y:", "sans-bold");
    auto y_translate = new nanogui::IntBox<int>(translatePanel);
    y_translate->set_editable(true);
    y_translate->set_fixed_size(nanogui::Vector2i(100, 20));
    y_translate->set_value(0);
    y_translate->set_default_value("0");
    y_translate->set_font_size(16);
    y_translate->set_format("[-]?[0-9]*");
    y_translate->set_spinnable(true);
    y_translate->set_value_increment(1);

    new nanogui::Label(translatePanel, "Z:", "sans-bold");
    auto z_translate = new nanogui::IntBox<int>(translatePanel);
    z_translate->set_editable(true);
    z_translate->set_fixed_size(nanogui::Vector2i(100, 20));
    z_translate->set_value(0);
    z_translate->set_default_value("0");
    z_translate->set_font_size(16);
    z_translate->set_format("[-]?[0-9]*");
    z_translate->set_spinnable(true);
    z_translate->set_value_increment(1);

    // Rotation section
    new nanogui::Label(this, "Rotation", "sans-bold");

    Widget *rotatePanel = new Widget(this);
    rotatePanel->set_layout(new nanogui::GridLayout(nanogui::Orientation::Vertical, 2,
                                                    nanogui::Alignment::Middle, 15, 5));

    new nanogui::Label(rotatePanel, "Pitch", "sans-bold");
    auto pitch_rotate = new nanogui::IntBox<int>(rotatePanel);
    pitch_rotate->set_editable(true);
    pitch_rotate->set_fixed_size(nanogui::Vector2i(100, 20));
    pitch_rotate->set_value(0);
    pitch_rotate->set_default_value("0");
    pitch_rotate->set_font_size(16);
    pitch_rotate->set_format("[-]?[0-9]*");
    pitch_rotate->set_spinnable(true);
    pitch_rotate->set_value_increment(1);

    new nanogui::Label(rotatePanel, "Yaw", "sans-bold");
    auto yaw_rotate = new nanogui::IntBox<int>(rotatePanel);
    yaw_rotate->set_editable(true);
    yaw_rotate->set_fixed_size(nanogui::Vector2i(100, 20));
    yaw_rotate->set_value(0);
    yaw_rotate->set_default_value("0");
    yaw_rotate->set_font_size(16);
    yaw_rotate->set_format("[-]?[0-9]*");
    yaw_rotate->set_spinnable(true);
    yaw_rotate->set_value_increment(1);

    new nanogui::Label(rotatePanel, "Roll", "sans-bold");
    auto roll_rotate = new nanogui::IntBox<int>(rotatePanel);
    roll_rotate->set_editable(true);
    roll_rotate->set_fixed_size(nanogui::Vector2i(100, 20));
    roll_rotate->set_value(0);
    roll_rotate->set_default_value("0");
    roll_rotate->set_font_size(16);
    roll_rotate->set_format("[-]?[0-9]*");
    roll_rotate->set_spinnable(true);
    roll_rotate->set_value_increment(1);

    // Scale section
    new nanogui::Label(this, "Scale", "sans-bold");

    Widget *scalePanel = new Widget(this);
    scalePanel->set_layout(new nanogui::GridLayout(nanogui::Orientation::Vertical, 2,
                                                   nanogui::Alignment::Middle, 15, 5));

    new nanogui::Label(scalePanel, "X", "sans-bold");
    auto x_scale = new nanogui::IntBox<int>(scalePanel);
    x_scale->set_editable(true);
    x_scale->set_fixed_size(nanogui::Vector2i(100, 20));
    x_scale->set_value(1);
    x_scale->set_default_value("1");
    x_scale->set_font_size(16);
    x_scale->set_format("[1-9][0-9]*");
    x_scale->set_spinnable(true);
    x_scale->set_value_increment(1);

    new nanogui::Label(scalePanel, "Y", "sans-bold");
    auto y_scale = new nanogui::IntBox<int>(scalePanel);
    y_scale->set_editable(true);
    y_scale->set_fixed_size(nanogui::Vector2i(100, 20));
    y_scale->set_value(1);
    y_scale->set_default_value("1");
    y_scale->set_font_size(16);
    y_scale->set_format("[1-9][0-9]*");
    y_scale->set_spinnable(true);
    y_scale->set_value_increment(1);

    new nanogui::Label(scalePanel, "Z", "sans-bold");
    auto z_scale = new nanogui::IntBox<int>(scalePanel);
    z_scale->set_editable(true);
    z_scale->set_fixed_size(nanogui::Vector2i(100, 20));
    z_scale->set_value(1);
    z_scale->set_default_value("1");
    z_scale->set_font_size(16);
    z_scale->set_format("[1-9][0-9]*");
    z_scale->set_spinnable(true);
    z_scale->set_value_increment(1);
}
