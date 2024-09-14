#pragma once

#include "actors/IActorSelectedCallback.hpp"
#include "graphics/drawing/Drawable.hpp"
#include "graphics/shading/ShaderWrapper.hpp"

#include <memory>
#include <optional>
#include <functional>

class IActorSelectedRegistry;
class IActorVisualManager;
class ActorManager;
class Canvas;
class CameraManager;
class GizmoManager;
class Grid;
class MeshActorLoader;
class ScenePanel;
class ShaderManager;

// UiManager class definition
class UiManager : public IActorSelectedCallback, public Drawable {
public:
	UiManager(IActorSelectedRegistry& registry, IActorVisualManager& actorVisualManager, ActorManager& actorManager, MeshActorLoader& meshActorLoader, ShaderManager& shaderManager, ScenePanel& scenePanel, Canvas& canvas, nanogui::Widget& toolbox, nanogui::Widget& statusBar, CameraManager& cameraManager, std::function<void(std::function<void(int, int)>)> applicationClickRegistrator);
	~UiManager();
	
	void OnActorSelected(Actor& actor) override;
	
	void draw();
	
	void draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view,
					  const nanogui::Matrix4f& projection) override;

private:
	IActorSelectedRegistry& mRegistry;
	ActorManager& mActorManager;
	std::optional<std::reference_wrapper<Actor>> mActiveActor;

	ShaderWrapper mShader;
	
	std::unique_ptr<GizmoManager> mGizmoManager;
	
	std::unique_ptr<Grid> mGrid;
};
