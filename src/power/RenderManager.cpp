#include "RenderManager.hpp"

#include "graphics/drawing/Drawable.hpp"

void RenderManager::render(Canvas& canvas) {
    for (auto drawable : drawables) {
        drawable.get().draw_content(canvas);
    }
    drawables.clear();
}

void RenderManager::add_drawable(std::reference_wrapper<Drawable> drawable) {
    drawables.push_back(drawable);
}

