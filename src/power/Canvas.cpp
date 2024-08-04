#include "Canvas.hpp"

#include "actors/ActorManager.hpp"
#include "graphics/drawing/Drawable.hpp"

#include <nanogui/opengl.h>
#include <nanogui/renderpass.h>

#include <iostream>

Canvas::Canvas(Widget* parent, nanogui::Color backgroundColor) : nanogui::Canvas(parent, 1, true, true) {
    set_background_color(backgroundColor);
    set_fixed_size(parent->fixed_size());
}

void Canvas::draw_contents() {
    for (auto& callback : mDrawCallbacks) {
        callback();
    }
}

void Canvas::register_draw_callback(std::function<void()> callback) {
    mDrawCallbacks.push_back(callback);
}
