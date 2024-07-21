#pragma once

#include <nanogui/canvas.h>

class ActorManager;

class Canvas : public nanogui::Canvas
{
   public:
    Canvas(Widget* parent, ActorManager& actorManager, nanogui::Color backgroundColor,
           nanogui::Vector2i size);

    virtual void draw_contents() override;

   private:
    void visit(ActorManager& renderManager);

    ActorManager& mActorManager;
};
