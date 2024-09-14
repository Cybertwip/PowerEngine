#include "GizmoManager.hpp"


#include <nanogui/icons.h>
#include <nanogui/opengl.h>


#include <cmath>

#include "ShaderManager.hpp"
#include "actors/Actor.hpp"
#include "actors/ActorManager.hpp"
#include "components/TransformComponent.hpp"

#include "gizmo/TranslationGizmo.hpp"
#include "gizmo/RotationGizmo.hpp"
#include "gizmo/ScaleGizmo.hpp"


GizmoManager::GizmoManager(nanogui::Widget& parent, ShaderManager& shaderManager, ActorManager& actorManager)
: mShaderManager(shaderManager), mActorManager(actorManager) {
	mTranslationGizmo = std::make_unique<TranslationGizmo>(mShaderManager);
	mRotationGizmo = std::make_unique<RotationGizmo>(mShaderManager);
	mScaleGizmo = std::make_unique<ScaleGizmo>(mShaderManager);
	
	// Translation Button
	translationButton = new nanogui::ToolButton(&parent, FA_ARROWS_ALT);
	translationButton->set_flags(nanogui::Button::ToggleButton);
	translationButton->set_change_callback([this](bool state) {
		if (state) {
			set_mode(GizmoMode::Translation);
			// Ensure other buttons are not active
			rotationButton->set_pushed(false);
			scaleButton->set_pushed(false);
		}
	});
	
	// Rotation Button
	rotationButton = new nanogui::ToolButton(&parent, FA_REDO);
	rotationButton->set_flags(nanogui::Button::ToggleButton);
	rotationButton->set_change_callback([this](bool state) {
		if (state) {
			set_mode(GizmoMode::Rotation);
			// Ensure other buttons are not active
			translationButton->set_pushed(false);
			scaleButton->set_pushed(false);
		}
	});
	
	// Scale Button
	scaleButton = new nanogui::ToolButton(&parent, FA_EXPAND_ARROWS_ALT);
	scaleButton->set_flags(nanogui::Button::ToggleButton);
	scaleButton->set_change_callback([this](bool state) {
		if (state) {
			set_mode(GizmoMode::Scale);
			// Ensure other buttons are not active
			translationButton->set_pushed(false);
			rotationButton->set_pushed(false);
		}
	});
	
	// Set default mode button
	translationButton->set_pushed(true);  // Set default to Translation
}

void GizmoManager::select(int gizmoId) {
	mTranslationGizmo->select(gizmoId);
	mRotationGizmo->select(gizmoId);
	mScaleGizmo->select(gizmoId);
}

void GizmoManager::hover(int gizmoId) {
	mTranslationGizmo->hover(gizmoId);
	mRotationGizmo->hover(gizmoId);
	mScaleGizmo->hover(gizmoId);
}

void GizmoManager::select(std::optional<std::reference_wrapper<Actor>> actor) {
	mActiveActor = actor;
}

void GizmoManager::transform(float px, float py) {
	mTranslationGizmo->transform(mActiveActor, px, py);
	
	mRotationGizmo->transform(mActiveActor, ((px + py) / 2.0f));

	mScaleGizmo->transform(mActiveActor, px, py);
}

void GizmoManager::draw() { mActorManager.visit(*this); }

void GizmoManager::draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view,
								const nanogui::Matrix4f& projection) {
	
	if (mActiveActor.has_value()) {		
		switch (mCurrentMode) {
			case GizmoMode::Translation:
				mTranslationGizmo->draw_content(mActiveActor, model, view, projection);
				break;
			case GizmoMode::Rotation:
				mRotationGizmo->draw_content(mActiveActor, model, view, projection);
				break;
			case GizmoMode::Scale:
				mScaleGizmo->draw_content(mActiveActor, model, view, projection);
				break;
		}
		
		
	}
}

