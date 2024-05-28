#ifndef UI_IMGUI_HIERARCHY_LAYER_H
#define UI_IMGUI_HIERARCHY_LAYER_H

#include <memory>
#include <imgui.h>
#include "ui_context.h"

namespace anim
{
    class Entity;
}
class Scene;

namespace ui
{
    class HierarchyLayer
    {
    public:
        HierarchyLayer();
        ~HierarchyLayer();
        void draw(Scene *scene, UiContext &ui_context);

    private:
		std::shared_ptr<anim::Entity>  draw_tree_node(std::shared_ptr<anim::Entity>  entity_node, const ImGuiTreeNodeFlags &node_flags, Scene *scene, UiContext& ui_context, int depth = 0);
        int selected_id_{-2};
    };

}
#endif
