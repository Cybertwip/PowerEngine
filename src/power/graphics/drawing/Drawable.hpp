#pragma once

#include <nanogui/vector.h>

class Canvas;
class CameraManager;

class Drawable {
public:
	virtual ~Drawable() = default;
	virtual void draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view, const nanogui::Matrix4f& projection) = 0;
};
