#pragma once

#include "graphics/drawing/Drawable.hpp"

class CameraManager;

class DrawableComponent : public Drawable {
public:
    DrawableComponent(Drawable& drawable);
    
    void draw_content(CameraManager& cameraManager) override;
    
private:
    Drawable& mDrawable;
};
