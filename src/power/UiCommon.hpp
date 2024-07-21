#pragma once

namespace nanogui{
class Widget;
}

class ScenePanel;
class TransformPanel;
class HierarchyPanel;

class UiCommon {
public:
    UiCommon(nanogui::Widget& parent);
    
    ScenePanel& scene_panel() {
        return *mScenePanel;
    }

    TransformPanel& transform_panel() {
        return *mTransformPanel;
    }

    HierarchyPanel& hierarchy_panel() {
        return *mHierarchyPanel;
    }

private:
    ScenePanel* mScenePanel;
    TransformPanel* mTransformPanel;
    HierarchyPanel* mHierarchyPanel;
};
