#include "Canvas.hpp"
#include "RenderManager.hpp"

#include "graphics/drawing/Drawable.hpp"

Canvas::Canvas(Widget *parent, RenderManager& renderManager) :
nanogui::Canvas(parent, 1),
mRenderManager(renderManager) {
}

void Canvas::draw_contents() {
    visit(mRenderManager);
}

void Canvas::visit(RenderManager& renderManager) {
    renderManager.render(*this);
}
