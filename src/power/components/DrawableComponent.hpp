#pragma once

#include "graphics/drawing/Drawable.hpp"

#include <memory>

class CameraManager;

class DrawableComponent : public Drawable {
public:
    DrawableComponent(std::unique_ptr<Drawable> drawable);
    
    void draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view, const nanogui::Matrix4f& projection) override;
    
private:
	std::unique_ptr<Drawable> mDrawable;
};
