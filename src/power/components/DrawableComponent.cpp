#include "DrawableComponent.hpp"

DrawableComponent::DrawableComponent(Drawable& drawable) : mDrawable(drawable) {}

void DrawableComponent::draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view,
                                     const nanogui::Matrix4f& projection) {
    mDrawable.draw_content(model, view, projection);
}
