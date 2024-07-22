#pragma once

#include <functional>
#include <vector>

namespace nanogui{
class Widget;
}

class Actor;
class HierarchyPanel;
class ScenePanel;
class StatusBarPanel;
class TransformPanel;


class UiCommon {
public:
    UiCommon(nanogui::Widget& parent);
    
    void attach_actors(const std::vector<std::reference_wrapper<Actor>> &actors);
    
    HierarchyPanel& hierarchy_panel() {
        return *mHierarchyPanel;
    }

    ScenePanel& scene_panel() {
        return *mScenePanel;
    }

    StatusBarPanel& status_bar_panel() {
        return *mStatusBarPanel;
    }

    TransformPanel& transform_panel() {
        return *mTransformPanel;
    }
    
    void update();

private:
    ScenePanel* mScenePanel;
    TransformPanel* mTransformPanel;
    HierarchyPanel* mHierarchyPanel;
    StatusBarPanel* mStatusBarPanel;
};
