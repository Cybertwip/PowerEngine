#include "Canvas.hpp"

#include "actors/ActorManager.hpp"

#include "graphics/drawing/Drawable.hpp"

Canvas::Canvas(Widget* parent, ActorManager& actorManager, nanogui::Color backgroundColor,
               nanogui::Vector2i size)
    : nanogui::Canvas(parent, 1), mActorManager(actorManager) {
    set_background_color(backgroundColor);
    set_fixed_size(size);
}

void Canvas::draw_contents() { visit(mActorManager); }

void Canvas::visit(ActorManager& actorManager) { actorManager.draw(); }
