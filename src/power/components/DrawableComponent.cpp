#include "DrawableComponent.hpp"

DrawableComponent::DrawableComponent(Drawable& drawable) : mDrawable(drawable) {
    
}

void DrawableComponent::draw_content(CameraManager& cameraManager) {
    mDrawable.draw_content(cameraManager);
}
