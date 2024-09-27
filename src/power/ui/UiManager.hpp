#pragma once

#include "actors/IActorSelectedCallback.hpp"
#include "graphics/drawing/Drawable.hpp"
#include "graphics/shading/ShaderWrapper.hpp"

#include <glm/glm.hpp>

#include <memory>
#include <optional>
#include <functional>

namespace nanogui {
class RenderPass;
}

class IActorSelectedRegistry;
class IActorVisualManager;
class ActorManager;
class AnimationPanel;
class BatchUnit;
class Canvas;
class CameraManager;
class GizmoManager;
class Grid;
class MeshActorLoader;
class ScenePanel;
class SceneTimeBar;
class ShaderManager;
class StatusBarPanel;

// UiManager class definition
class UiManager : public IActorSelectedCallback, public Drawable {
public:
	UiManager(IActorSelectedRegistry& registry, IActorVisualManager& actorVisualManager, ActorManager& actorManager, MeshActorLoader& meshActorLoader, ShaderManager& shaderManager, ScenePanel& scenePanel, Canvas& canvas, nanogui::Widget& toolbox, nanogui::Widget& statusBar, AnimationPanel& animationPanel, CameraManager& cameraManager, std::function<void(std::function<void(int, int)>)> applicationClickRegistrator);
	~UiManager();
	
	void OnActorSelected(std::optional<std::reference_wrapper<Actor>> actor) override;
	
	void draw();
	
	void draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view,
					  const nanogui::Matrix4f& projection) override;

	StatusBarPanel& status_bar_panel() {
		return *mStatusBarPanel;
	}
private:
	IActorSelectedRegistry& mRegistry;
	ActorManager& mActorManager;
	std::optional<std::reference_wrapper<Actor>> mActiveActor;

	ShaderManager& mShaderManager;

	std::unique_ptr<Grid> mGrid;
	
	MeshActorLoader& mMeshActorLoader;
	std::unique_ptr<GizmoManager> mGizmoManager;

	Canvas& mCanvas;
	AnimationPanel& mAnimationPanel;
	nanogui::RenderPass* mRenderPass;
	
	SceneTimeBar* mSceneTimeBar;
	
	glm::vec4 mSelectionColor;
	
	StatusBarPanel* mStatusBarPanel;
	
};
