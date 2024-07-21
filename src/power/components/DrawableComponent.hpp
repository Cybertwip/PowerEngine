#pragma once

#include "graphics/drawing/Drawable.hpp"

class CameraManager;

class DrawableComponent : public Drawable {
public:
    DrawableComponent(Drawable& drawable);
    
    void draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view, const nanogui::Matrix4f& projection) override;
    
private:
    Drawable& mDrawable;
};
