#ifndef UI_IMGUI_COMPONENT_LAYER_H
#define UI_IMGUI_COMPONENT_LAYER_H

#include "ui_context.h"

namespace glcpp{
class Camera;
}

class Scene;
namespace anim
{
    class SharedResources;
    class Entity;
    class TransformComponent;
    class AnimationComponent;
    class MeshComponent;
}

namespace ui
{
    class ComponentLayer
    {
    public:
        ComponentLayer();
        ~ComponentLayer();
        void draw(ComponentContext &context, Scene *scene);

    private:
        void draw_animation(ComponentContext &context, anim::SharedResources *shared_resource, std::shared_ptr<anim::Entity> entity, const anim::AnimationComponent *animation);
        void draw_transform(std::shared_ptr<anim::Entity> entity);
        void draw_transform_reset_button(anim::TransformComponent &transform);
        void draw_mesh(anim::MeshComponent *mesh);
    };
}

#endif
