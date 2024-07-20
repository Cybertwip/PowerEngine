#include "Canvas.hpp"

#include "RenderManager.hpp"
#include "graphics/drawing/Drawable.hpp"

Canvas::Canvas(Widget* parent, RenderManager& renderManager, nanogui::Color backgroundColor,
               nanogui::Vector2i size)
    : nanogui::Canvas(parent, 1), mRenderManager(renderManager) {
    set_background_color(backgroundColor);
    set_fixed_size(size);
}

void Canvas::draw_contents() { visit(mRenderManager); }

void Canvas::visit(RenderManager& renderManager) { renderManager.render(*this); }
