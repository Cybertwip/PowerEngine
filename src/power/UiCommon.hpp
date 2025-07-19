#pragma once

#include <nanogui/widget.h>

#include <functional>
#include <vector>
#include <memory>

namespace nanogui{
class Widget;
class Window;
}

class Actor;
class ActorManager;
class AnimationPanel;
class BlueprintManager;
class CameraManager;
class CameraPanel;
class AnimationTimeProvider;
class HierarchyPanel;
class HierarchyToolPanel;
class ScenePanel;
//class SceneTimeBar;
class ShaderManager;
class StatusBarPanel;
class TransformPanel;

class UiCommon {
public:
    UiCommon(nanogui::Widget& parent, nanogui::Screen& screen, ActorManager& actorManager, AnimationTimeProvider& animationTimeProvider);
        
	std::shared_ptr<HierarchyPanel> hierarchy_panel() {
        return mHierarchyPanel;
    }

    std::shared_ptr<ScenePanel> scene_panel() {
        return mScenePanel;
    }

	std::shared_ptr<nanogui::Widget> status_bar() {
        return mStatusBar;
    }

	std::shared_ptr<TransformPanel> transform_panel() {
		return mTransformPanel;
	}
	
	std::shared_ptr<CameraPanel> camera_panel() {
		return mCameraPanel;
	}

	std::shared_ptr<AnimationPanel> animation_panel() {
		return mAnimationPanel;
	}
	
//	std::shared_ptr<SceneTimeBar> scene_time_bar() {
//		return mSceneTimeBar;
//	}

private:
	std::shared_ptr<nanogui::Window> mMainWrapper;
	std::shared_ptr<nanogui::Window> mSceneWrapper;
	std::shared_ptr<nanogui::Window> mLeftWrapper;
	std::shared_ptr<nanogui::Window> mRightWrapper;
	std::shared_ptr<ScenePanel> mScenePanel;
	std::shared_ptr<TransformPanel> mTransformPanel;
	std::shared_ptr<CameraPanel> mCameraPanel;
	std::shared_ptr<AnimationPanel> mAnimationPanel;
	std::shared_ptr<HierarchyPanel> mHierarchyPanel;
	std::shared_ptr<nanogui::Widget> mStatusBar;
//	std::shared_ptr<SceneTimeBar> mSceneTimeBar;
	
	ActorManager& mActorManager;
	AnimationTimeProvider& mAnimationTimeProvider;
};
