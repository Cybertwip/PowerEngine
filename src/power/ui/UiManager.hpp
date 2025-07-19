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
class Screen;
}

class IActorSelectedRegistry;
class IActorVisualManager;
class ActorManager;
class AnimationPanel;
class AnimationTimeProvider;
class BatchUnit;
class Canvas;
class CameraManager;
class GizmoManager;
class Grid;
class MeshActorLoader;
class ScenePanel;
//class SceneTimeBar;
class ShaderManager;
class StatusBarPanel;

// ==============================
// 5. UiManager Class Declaration
// ==============================
class UiManager : public IActorSelectedCallback, public Drawable {
public:
	UiManager(nanogui::Screen& screen, std::shared_ptr<IActorSelectedRegistry> registry,
			  std::shared_ptr<IActorVisualManager> actorVisualManager,
			  ActorManager& actorManager,
			  MeshActorLoader& meshActorLoader,
			  MeshActorLoader& gizmoActorLoader,
			  ShaderManager& shaderManager,
			  std::shared_ptr<ScenePanel> scenePanel,
			  std::shared_ptr<Canvas> canvas,
			  std::shared_ptr<nanogui::Widget> statusBar,
			  std::shared_ptr<AnimationPanel> animationPanel,
//			  std::shared_ptr<SceneTimeBar> sceneTimeBar,
			  AnimationTimeProvider& animationTimeProvider,
			  CameraManager& cameraManager,
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
	std::shared_ptr<StatusBarPanel> status_bar_panel();
	void export_movie(const std::string& path);
	bool is_movie_exporting() const;
	void remove_active_actor();
	void process_events();
	
private:
	// ==============================
	// Member Variables (Ordered by Dependencies)
	// ==============================
	std::shared_ptr<IActorSelectedRegistry> mRegistry;
	std::shared_ptr<IActorVisualManager> mActorVisualManager;
	ActorManager& mActorManager;
	ShaderManager& mShaderManager;
	std::unique_ptr<Grid> mGrid;
	MeshActorLoader& mMeshActorLoader;
	MeshActorLoader& mGizmoActorLoader;
	GizmoManager& mGizmoManager;
	CameraManager& mCameraManager;
	std::shared_ptr<Canvas> mCanvas;
	std::shared_ptr<AnimationPanel> mAnimationPanel;
	std::shared_ptr<nanogui::RenderPass> mRenderPass;
//	std::shared_ptr<SceneTimeBar> mSceneTimeBar;
	glm::vec4 mSelectionColor;
	std::shared_ptr<StatusBarPanel> mStatusBarPanel;
	bool mIsMovieExporting;
	std::string mMovieExportFile;
	std::string mMovieExportDirectory;
	int mFrameCounter;
	int mFramePadding; // Number of digits for frame numbering
	
	std::optional<std::reference_wrapper<Actor>> mActiveActor;

};
