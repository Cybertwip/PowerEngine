#pragma once

#include <functional>
#include <vector>
#include <memory>

namespace nanogui{
class Widget;
}

class Actor;
class ActorManager;
class AnimationPanel;
class HierarchyPanel;
class ScenePanel;
class ShaderManager;
class StatusBarPanel;
class TransformPanel;
class UiManager;

class UiCommon {
public:
    UiCommon(nanogui::Widget& parent, ActorManager& actorManager);
        
    HierarchyPanel& hierarchy_panel() {
        return *mHierarchyPanel;
    }

    ScenePanel& scene_panel() {
        return *mScenePanel;
    }

    nanogui::Widget& status_bar() {
        return *mStatusBar;
    }

	TransformPanel& transform_panel() {
		return *mTransformPanel;
	}

	AnimationPanel& animation_panel() {
		return *mAnimationPanel;
	}

	nanogui::Widget& toolbox() {
		return *mToolbox;
	}
	    
private:
    ScenePanel* mScenePanel;
	TransformPanel* mTransformPanel;
	AnimationPanel* mAnimationPanel;
    HierarchyPanel* mHierarchyPanel;
	nanogui::Widget* mStatusBar;
	StatusBarPanel* mStatusBarPanel;
	nanogui::Widget* mToolbox;
};
