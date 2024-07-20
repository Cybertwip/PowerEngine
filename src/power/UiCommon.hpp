#pragma once

namespace nanogui{
class Widget;
}

class ScenePanel;
class TransformPanel;

class UiCommon {
public:
    UiCommon(nanogui::Widget& parent);
    
    ScenePanel& scene_panel() {
        return *mScenePanel;
    }

    TransformPanel& transform_panel() {
        return *mTransformPanel;
    }

private:
    ScenePanel* mScenePanel;
    TransformPanel* mTransformPanel;
};
