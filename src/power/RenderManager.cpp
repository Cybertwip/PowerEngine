#include "RenderManager.hpp"

#include "graphics/drawing/Drawable.hpp"

RenderManager::RenderManager(CameraManager& cameraManager) : mCameraManager(cameraManager) {}
void RenderManager::render(Canvas& canvas) {
    for (auto drawable : drawables) {
        drawable.get().draw_content(mCameraManager);
    }
    drawables.clear();
}

void RenderManager::add_drawable(std::reference_wrapper<Drawable> drawable) {
    drawables.push_back(drawable);
}
