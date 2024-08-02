#pragma once

#include "Drawable.hpp"

#include <nanogui/vector.h>

class NullDrawable : public Drawable {
public:
	NullDrawable() = default;
    void draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view, const nanogui::Matrix4f& projection) override {}
};
