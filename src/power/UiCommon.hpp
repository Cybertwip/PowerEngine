#pragma once

#include <functional>
#include <vector>

namespace nanogui{
class Widget;
}

class Actor;
class ScenePanel;
class TransformPanel;
class HierarchyPanel;

class UiCommon {
public:
    UiCommon(nanogui::Widget& parent);
    
    void attach_actors(const std::vector<std::reference_wrapper<Actor>> &actors);
    
    ScenePanel& scene_panel() {
        return *mScenePanel;
    }

    TransformPanel& transform_panel() {
        return *mTransformPanel;
    }

    HierarchyPanel& hierarchy_panel() {
        return *mHierarchyPanel;
    }
    
    void update();

private:
    ScenePanel* mScenePanel;
    TransformPanel* mTransformPanel;
    HierarchyPanel* mHierarchyPanel;
};
