#pragma once

#include "graphics/drawing/Drawable.hpp"

#include "gizmo/TranslationGizmo.hpp"
#include "gizmo/RotationGizmo.hpp"
#include "gizmo/ScaleGizmo.hpp"

#include <nanogui/nanogui.h>

#include <functional>
#include <optional>
#include <vector>
#include <memory>

class Actor;
class ActorManager;
class MeshActorLoader;
class ShaderManager;

class GizmoManager : public Drawable {
	enum class GizmoMode {
		None,
		Translation,
		Rotation,
		Scale
	};

public:
	enum class GizmoAxis : int8_t {
		None = 0,
		X = -1,
		Y = -2,
		Z = -3
	};
	
    GizmoManager(nanogui::Widget& parent, ShaderManager& shaderManager, ActorManager& actorManager, MeshActorLoader& meshActorLoader);
	~GizmoManager() = default;
	
	void select(GizmoAxis gizmoId);

	void hover(GizmoAxis gizmoId);

	void select(std::optional<std::reference_wrapper<Actor>> actor);
    
	void translate(float px, float py);
	void rotate(float px, float py);
	void scale(float px, float py);

	void transform(float px, float py);
	
    void draw();
    
    void draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view, const nanogui::Matrix4f& projection) override;

private:
	void set_mode(GizmoMode mode);

    ShaderManager& mShaderManager;
    ActorManager& mActorManager;
	MeshActorLoader& mMeshActorLoader;
	std::unique_ptr<ShaderWrapper> mShader;
	Actor& mTranslationGizmo;
	Actor& mRotationGizmo;
//	Actor& mScaleGizmo;

	std::optional<std::reference_wrapper<Actor>> mActiveActor;
	
	GizmoMode mCurrentMode = GizmoMode::Translation;  // Default mode is Translation

	GizmoAxis mGizmoAxis = GizmoAxis::None;
	
	nanogui::Button* translationButton;
	nanogui::Button* rotationButton;
	nanogui::Button* scaleButton;
	
	std::optional<std::reference_wrapper<Actor>> mActiveGizmo;
};

