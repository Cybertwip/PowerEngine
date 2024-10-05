#pragma once

// ==============================
// 1. Standard Library Includes
// ==============================
#include <memory>
#include <optional>
#include <functional>
#include <string>
#include <vector>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <cstdio>

// ==============================
// 2. Third-Party Library Includes
// ==============================
#include <glm/glm.hpp>
#include <nanogui/nanogui.h>
#include <nanovg.h>

// ==============================
// 3. Project-Specific Includes
// ==============================
#include "actors/IActorSelectedCallback.hpp"
#include "graphics/drawing/Drawable.hpp"
#include "graphics/shading/ShaderWrapper.hpp"

// ==============================
// 4. Forward Declarations
// ==============================
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
class DeepMotionApiClient;
class GizmoManager;
class Grid;
class MeshActorLoader;
class ScenePanel;
class SceneTimeBar;
class ShaderManager;
class StatusBarPanel;

// ==============================
// 5. UiManager Class Declaration
// ==============================
class UiManager : public IActorSelectedCallback, public Drawable {
public:
	UiManager(IActorSelectedRegistry& registry,
			  IActorVisualManager& actorVisualManager,
			  ActorManager& actorManager,
			  MeshActorLoader& meshActorLoader,
			  ShaderManager& shaderManager,
			  ScenePanel& scenePanel,
			  Canvas& canvas,
			  nanogui::Widget& toolbox,
			  nanogui::Widget& statusBar,
			  AnimationPanel& animationPanel,
			  SceneTimeBar& sceneTimeBar,
			  CameraManager& cameraManager,
			  DeepMotionApiClient& deepMotionApiClient,
			  GizmoManager& gizmoManager,
			  std::function<void(std::function<void(int, int)>)> applicationClickRegistrator);
	
	~UiManager();
	
	// Overridden Methods
	void OnActorSelected(std::optional<std::reference_wrapper<Actor>> actor) override;
	void draw();
	void draw_content(const nanogui::Matrix4f& model,
					  const nanogui::Matrix4f& view,
					  const nanogui::Matrix4f& projection) override;
	
	// Public Interface
	StatusBarPanel& status_bar_panel();
	void export_movie(const std::string& path);
	bool is_movie_exporting() const;
	void remove_active_actor();
	void process_events();
	
private:
	// ==============================
	// Member Variables (Ordered by Dependencies)
	// ==============================
	IActorSelectedRegistry& mRegistry;
	IActorVisualManager& mActorVisualManager;
	ActorManager& mActorManager;
	ShaderManager& mShaderManager;
	std::unique_ptr<Grid> mGrid;
	MeshActorLoader& mMeshActorLoader;
	GizmoManager& mGizmoManager;
	CameraManager& mCameraManager;
	Canvas& mCanvas;
	AnimationPanel& mAnimationPanel;
	nanogui::RenderPass* mRenderPass;
	SceneTimeBar& mSceneTimeBar;
	glm::vec4 mSelectionColor;
	StatusBarPanel* mStatusBarPanel;
	bool mIsMovieExporting;
	std::string mMovieExportFile;
	std::string mMovieExportDirectory;
	int mFrameCounter;
	int mFramePadding; // Number of digits for frame numbering
	
	std::optional<std::reference_wrapper<Actor>> mActiveActor;

};
