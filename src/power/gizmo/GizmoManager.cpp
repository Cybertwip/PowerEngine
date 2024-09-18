#include "GizmoManager.hpp"


#include <nanogui/icons.h>

#if defined(NANOGUI_USE_OPENGL) || defined(NANOGUI_USE_GLES)
#include <nanogui/opengl.h>

#elif defined(NANOGUI_USE_METAL)
#include "MetalHelper.hpp"
#endif

#include <cmath>

#include "MeshActorLoader.hpp"
#include "ShaderManager.hpp"
#include "actors/Actor.hpp"
#include "actors/ActorManager.hpp"
#include "components/ColorComponent.hpp"
#include "components/TransformComponent.hpp"

#include "gizmo/TranslationGizmo.hpp"
#include "gizmo/RotationGizmo.hpp"
#include "gizmo/ScaleGizmo.hpp"

GizmoManager::GizmoManager(nanogui::Widget& parent, ShaderManager& shaderManager, ActorManager& actorManager, MeshActorLoader& meshActorLoader)
: mShaderManager(shaderManager), mActorManager(actorManager),
mMeshActorLoader(meshActorLoader),
mShader(std::make_unique<ShaderWrapper>(*shaderManager.get_shader("gizmo"))),
mTranslationGizmo(mMeshActorLoader.create_actor("models/Gizmo/Translation.fbx", *mShader)),
mRotationGizmo(mMeshActorLoader.create_actor("models/Gizmo/Rotation.fbx", *mShader))
/*mScaleGizmo(mMeshActorLoader.create_actor("models/Gizmo/Translation.fbx"))*/ {
	
	// Translation Button
	mTranslationButton = new nanogui::Button(&parent, "", FA_ARROWS_ALT);
	mTranslationButton->set_callback([this]() {
		set_mode(GizmoMode::Translation);
	});
	
	// Rotation Button
	mRotationButton = new nanogui::Button(&parent, "", FA_REDO);
	mRotationButton->set_callback([this]() {
		set_mode(GizmoMode::Rotation);
	});
	
	// Scale Button
	mScaleButton = new nanogui::Button(&parent, "", FA_EXPAND_ARROWS_ALT);
	mScaleButton->set_callback([this]() {
		set_mode(GizmoMode::Scale);
	});

	select(std::nullopt);
	
	set_mode(GizmoMode::None);
}

void GizmoManager::select(GizmoAxis gizmoId) {
	mGizmoAxis = gizmoId;
	//	mTranslationGizmo->select(gizmoId);
	//	mRotationGizmo->select(gizmoId);
	//	mScaleGizmo->select(gizmoId);
}

void GizmoManager::hover(GizmoAxis gizmoId) {
	//	mTranslationGizmo->hover(gizmoId);
	//	mRotationGizmo->hover(gizmoId);
	//	mScaleGizmo->hover(gizmoId);
}

void GizmoManager::select(std::optional<std::reference_wrapper<Actor>> actor) {
	mActiveActor = actor;
	
	if (mActiveActor.has_value()) {
		set_mode(mCurrentMode);
	} else {
		mTranslationGizmo.get_component<ColorComponent>().set_visible(false);
		
		mRotationGizmo.get_component<ColorComponent>().set_visible(false);
	}
}

void GizmoManager::translate(float px, float py) {
	auto& actor = mActiveActor.value().get();
	
	switch (mGizmoAxis) {
		case GizmoAxis::X:{
			auto& transformComponent = actor.get_component<TransformComponent>();
			
			auto translation = transformComponent.get_translation();
			
			translation.x += px;
			
			transformComponent.set_translation(translation);
		}
			break;
			
		case GizmoAxis::Y:{
			auto& transformComponent = actor.get_component<TransformComponent>();
			
			auto translation = transformComponent.get_translation();
			
			translation.y += py;
			
			transformComponent.set_translation(translation);
		}
			break;
			
		case GizmoAxis::Z: {
			auto& transformComponent = actor.get_component<TransformComponent>();
			
			auto translation = transformComponent.get_translation();
			
			translation.z += px;
			
			transformComponent.set_translation(translation);
		}
			break;
			
		default:
			break;
	}}


void GizmoManager::rotate(float px, float py) {
	float angle = ((px + py) / 2.0f);
	
	auto& actor = mActiveActor.value().get();
	
	switch (mGizmoAxis) {
		case GizmoAxis::X:{
			auto& transformComponent = actor.get_component<TransformComponent>();
			
			transformComponent.rotate(glm::vec3(1.0f, 0.0f, 0.0f), angle); // Rotate around X-axis
		}
			break;
			
		case GizmoAxis::Y:{
			auto& transformComponent = actor.get_component<TransformComponent>();
			
			transformComponent.rotate(glm::vec3(0.0f, 0.0f, -1.0f), angle); // Rotate around Z-axis
		}
			break;
			
		case GizmoAxis::Z: {
			auto& transformComponent = actor.get_component<TransformComponent>();

			transformComponent.rotate(glm::vec3(0.0f, -1.0f, 0.0f), angle); // Rotate around Y-axis
		}
			break;
			
		default:
			break;
	}
	
}

void GizmoManager::scale(float px, float py) {
	auto& actor = mActiveActor.value().get();
	switch (mGizmoAxis) {
		case GizmoAxis::X:{
			auto& transformComponent = actor.get_component<TransformComponent>();
			
			auto scale = transformComponent.get_scale();
			
			scale.x += px;
			
			transformComponent.set_scale(scale);
		}
			break;
			
		case GizmoAxis::Y:{
			auto& transformComponent = actor.get_component<TransformComponent>();
			
			auto scale = transformComponent.get_scale();
			
			scale.y += py;
			
			transformComponent.set_scale(scale);
		}
			break;
			
		case GizmoAxis::Z: {
			auto& transformComponent = actor.get_component<TransformComponent>();
			
			auto scale = transformComponent.get_scale();
			
			scale.z += px;
			
			transformComponent.set_scale(scale);
		}
			break;
			
		default:
			break;
	}
}

void GizmoManager::transform(float px, float py) {
	if (mActiveActor.has_value()) {
		auto& actor = mActiveActor.value().get();
		
		switch (mCurrentMode) {
			case GizmoMode::Translation:{
				translate(px, py);
			}
				break;
			case GizmoMode::Rotation:{
				rotate(px, py);
			}
				break;
			case GizmoMode::Scale:{
				scale(px, py);
			}
				break;
				
			default:
				break;
		}
	}
}

void GizmoManager::set_mode(GizmoMode mode) {
	mCurrentMode = mode;
	
	mTranslationGizmo.get_component<ColorComponent>().set_visible(false);
	
	mRotationGizmo.get_component<ColorComponent>().set_visible(false);

	switch (mCurrentMode) {
		case GizmoMode::Translation:{
			mActiveGizmo = mTranslationGizmo;
		}
			break;
		case GizmoMode::Rotation:
			mActiveGizmo = mRotationGizmo;
			break;
		case GizmoMode::Scale:
			break;
		default:
			mActiveGizmo = std::nullopt;
			break;
	}

	if (mActiveActor.has_value() && mActiveGizmo.has_value()) {
		mActiveGizmo->get().get_component<ColorComponent>().set_visible(true);
	}
}

void GizmoManager::draw() {
	mActorManager.visit(*this);
}

void GizmoManager::draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view,
								const nanogui::Matrix4f& projection) {
	
	if (mActiveActor.has_value()) {
		if (mActiveGizmo.has_value()) {
			auto& actor = mActiveActor;
			
			auto& transformComponent = actor->get().get_component<TransformComponent>();
			
			auto translation = transformComponent.get_translation();
			auto rotation = transformComponent.get_rotation();
			
			auto viewInverse = view.inverse();
			glm::vec3 cameraPosition(view.m[3][0], view.m[3][1], view.m[3][2]);
			
			// Use GLM for translation matrix
			glm::mat4 actorTranslationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(translation.x, translation.y, -translation.z));
			
			float distance = glm::distance(cameraPosition, glm::vec3(translation.x, translation.y, translation.z)); // Now using actor's position for distance
			float visualScaleFactor = std::max(0.005f, distance * 0.005f);
			
			glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(visualScaleFactor, visualScaleFactor, visualScaleFactor));

			glm::mat4 rotationMatrix = glm::mat4(1.0f); // Identity matrix
			
			if (&mActiveGizmo->get() == &mTranslationGizmo) {
				rotationMatrix = glm::rotate(rotationMatrix, glm::radians(-90.0f), glm::vec3(0, 1, 0));
			} else if (&mActiveGizmo->get() == &mRotationGizmo) {
				// First rotate around Z-axis by 90 degrees (as in your original code)
				
				// Then rotate around the Y-axis by another 90 degrees
				rotationMatrix = glm::rotate(rotationMatrix, glm::radians(270.0f), glm::vec3(0, 1, 0));
				
				rotationMatrix = glm::rotate(rotationMatrix, glm::radians(90.0f), glm::vec3(0, 0, 1));

			}

			// Apply transformations in order: rotation, translation, then scale
			auto gizmoModel = actorTranslationMatrix * rotationMatrix * scaleMatrix;
			
			auto gizmoMatrix = TransformComponent::glm_to_nanogui(gizmoModel);

			
			auto& drawable = mActiveGizmo->get().get_component<DrawableComponent>();
						
			drawable.draw_content(gizmoMatrix, view, projection);

		}

		

	}
}

