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
class ShaderManager;

class GizmoManager : public Drawable {
	enum class GizmoMode { Translation, Rotation, Scale };

public:
    GizmoManager(nanogui::Widget& parent, ShaderManager& shaderManager, ActorManager& actorManager);
	~GizmoManager() = default;
	
	void select(int gizmoId);

	void hover(int gizmoId);

	void select(std::optional<std::reference_wrapper<Actor>> actor);
    
	void transform(float px, float py);
	
    void draw();
    
    void draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view, const nanogui::Matrix4f& projection) override;

private:
	void set_mode(GizmoMode mode) { mCurrentMode = mode; }

    ShaderManager& mShaderManager;
    ActorManager& mActorManager;    
	std::unique_ptr<TranslationGizmo> mTranslationGizmo;
	std::unique_ptr<RotationGizmo> mRotationGizmo;
	std::unique_ptr<ScaleGizmo> mScaleGizmo;

	std::optional<std::reference_wrapper<Actor>> mActiveActor;
	
	GizmoMode mCurrentMode = GizmoMode::Translation;  // Default mode is Translation

	nanogui::Button* translationButton;
	nanogui::Button* rotationButton;
	nanogui::Button* scaleButton;
};

