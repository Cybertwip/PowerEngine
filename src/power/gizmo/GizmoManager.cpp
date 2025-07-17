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

#include "components/DrawableComponent.hpp"

#include "components/TransformComponent.hpp"



extern std::string get_resources_path();



GizmoManager::GizmoManager(nanogui::Widget& parent, ShaderManager& shaderManager, ActorManager& actorManager, MeshActorLoader& meshActorLoader)

: mDummyAnimationTimeProvider(60 * 30), mShaderManager(shaderManager), mActorManager(actorManager),

mMeshActorLoader(meshActorLoader),

mMeshShader(std::make_unique<ShaderWrapper>(shaderManager.get_shader("gizmo"))),

mSkinnedShader(std::make_unique<ShaderWrapper>(shaderManager.get_shader("skinned_mesh"))),

mTranslationGizmo(mMeshActorLoader.create_actor(get_resources_path() + "/internal/models/Gizmo/Translation.fbx", mDummyAnimationTimeProvider, mDummyAnimationTimeProvider, *mMeshShader, *mSkinnedShader)),

mRotationGizmo(mMeshActorLoader.create_actor(get_resources_path() + "/internal/models/Gizmo/Rotation.fbx", mDummyAnimationTimeProvider, mDummyAnimationTimeProvider, *mMeshShader, *mSkinnedShader)),

mScaleGizmo(mMeshActorLoader.create_actor(get_resources_path() + "/internal/models/Gizmo/Scale.fbx", mDummyAnimationTimeProvider, mDummyAnimationTimeProvider, *mMeshShader, *mSkinnedShader))

{
	
	
	// Translation Button
	
	mTranslationButton = std::make_shared<nanogui::Button>(parent, "", FA_ARROWS_ALT);
	
	mTranslationButton->set_callback([this]() {
		
		set_mode(GizmoMode::Translation);
		
	});
	
	
	// Rotation Button
	
	mRotationButton = std::make_shared<nanogui::Button>(parent, "", FA_REDO);
	
	mRotationButton->set_callback([this]() {
		
		set_mode(GizmoMode::Rotation);
		
	});
	
	
	// Scale Button
	
	mScaleButton = std::make_shared<nanogui::Button>(parent, "", FA_EXPAND_ARROWS_ALT);
	
	mScaleButton->set_callback([this]() {
		
		set_mode(GizmoMode::Scale);
		
	});
	
	
	mTranslationButton->set_size(nanogui::Vector2i(48, 48));
	
	mRotationButton->set_size(nanogui::Vector2i(48, 48));
	
	mScaleButton->set_size(nanogui::Vector2i(48, 48));
	
	
	
	mTranslationButton->set_position(nanogui::Vector2i(10,  parent.fixed_height() - mTranslationButton->height() - 10));
	
	mRotationButton->set_position(nanogui::Vector2i(mTranslationButton->position().x() + mTranslationButton->width() + 5, parent.fixed_height() - mRotationButton->height() - 10));
	
	mScaleButton->set_position(nanogui::Vector2i(mRotationButton->position().x() + mRotationButton->width() + 5, parent.fixed_height() - mScaleButton->height() - 10));
	
	
	select(std::nullopt);
	
	
	set_mode(GizmoMode::None);
	
}



void GizmoManager::select(GizmoAxis gizmoId) {
	
	mGizmoAxis = gizmoId;
	
}



void GizmoManager::hover(GizmoAxis gizmoId) {
	
	// mTranslationGizmo->hover(gizmoId);
	
	// mRotationGizmo->hover(gizmoId);
	
	// mScaleGizmo->hover(gizmoId);
	
}



void GizmoManager::select(std::optional<std::reference_wrapper<Actor>> actor) {
	
	mActiveActor = actor;
	
	
	if (mActiveActor.has_value()) {
		
		set_mode(mCurrentMode);
		
	} else {
		
		mTranslationGizmo.get_component<ColorComponent>().set_visible(false);
		
		
		mRotationGizmo.get_component<ColorComponent>().set_visible(false);
		
		
		mScaleGizmo.get_component<ColorComponent>().set_visible(false);
		
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
			
			
			translation.z -= py; // Use py instead of px
			
			
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
			
			
			transformComponent.rotate(glm::vec3(1.0f, 0.0f, 0.0f), -angle); // Rotate around X-axis
			
		}
			
			break;
			
			
		case GizmoAxis::Y:{
			
			auto& transformComponent = actor.get_component<TransformComponent>();
			
			
			transformComponent.rotate(glm::vec3(0.0f, 0.0f, -1.0f), -angle); // Rotate around Z-axis
			
		}
			
			break;
			
			
		case GizmoAxis::Z: {
			
			auto& transformComponent = actor.get_component<TransformComponent>();
			
			
			transformComponent.rotate(glm::vec3(0.0f, 1.0f, 0.0f), angle); // Rotate around Y-axis
			
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
			
			
			scale.z -= py;
			
			
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
				
				scale(px * 0.01f, py * 0.01f);
				
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
	
	
	mScaleGizmo.get_component<ColorComponent>().set_visible(false);
	
	
	switch (mCurrentMode) {
			
		case GizmoMode::Translation:{
			
			mActiveGizmo = mTranslationGizmo;
			
		}
			
			break;
			
		case GizmoMode::Rotation:
			
			mActiveGizmo = mRotationGizmo;
			
			break;
			
		case GizmoMode::Scale:
			
			mActiveGizmo = mScaleGizmo;
			
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
	
	
	if (mActiveActor.has_value() && mActiveGizmo.has_value()) {
		
		auto& actor = mActiveActor->get();
		
		auto& transformComponent = actor.get_component<TransformComponent>();
		
		
		// Get the actor's transformation properties
		
		auto translation = transformComponent.get_translation();
		
		auto rotation = transformComponent.get_rotation(); // Actor's rotation (glm::quat)
		
		
		glm::vec3 actorPosition(translation.x, translation.y, translation.z);
		
		
		// --- FIX 1: Correct Camera Position and Scaling ---
		
		// The camera's world position is in the inverse of the view matrix.
		
		// We assume nanogui matrices are column-major like OpenGL/GLM.
		
		// The translation vector is the 4th column.
		
		auto viewInverse = view.inverse();
		
		glm::vec3 cameraPosition(viewInverse.m[3][0], viewInverse.m[3][1], viewInverse.m[3][2]);
		
		
		// Calculate a scale factor to keep the gizmo a consistent size on screen
		
		float distance = glm::distance(cameraPosition, actorPosition);
		
		float visualScaleFactor = distance * 0.1f; // This factor may need tweaking
		
		glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(visualScaleFactor));
		
		
		// --- FIX 2: Correct Gizmo Rotation ---
		
		glm::mat4 actorTranslationMatrix = glm::translate(glm::mat4(1.0f), actorPosition);
		
		glm::mat4 actorRotationMatrix = glm::mat4_cast(rotation); // Convert actor's quaternion to a matrix
		
		
		glm::mat4 finalRotationMatrix;
		
		
		if (&mActiveGizmo->get() == &mRotationGizmo) {
			
			// The rotation gizmo MUST align with the actor's orientation.
			
			// We combine the actor's rotation with a correction for the model's default orientation.
			
			glm::mat4 modelCorrection = glm::mat4(1.0f);
			
			modelCorrection = glm::rotate(modelCorrection, glm::radians(180.0f), glm::vec3(0, 1, 0));
			
			modelCorrection = glm::rotate(modelCorrection, glm::radians(90.0f), glm::vec3(0, 0, 1));
			
			finalRotationMatrix = actorRotationMatrix * modelCorrection;
			
		} else {
			
			// For Translation and Scale, we'll keep the gizmo world-aligned (as the original code implied).
			
			// Its axes will not rotate with the actor. We only apply the model correction rotation.
			
			glm::mat4 modelCorrection = glm::mat4(1.0f);
			
			modelCorrection = glm::rotate(modelCorrection, glm::radians(90.0f), glm::vec3(1, 0, 0));
			
			modelCorrection = glm::rotate(modelCorrection, glm::radians(180.0f), glm::vec3(0, 1, 0));
			
			modelCorrection = glm::rotate(modelCorrection, glm::radians(90.0f), glm::vec3(0, 0, 1));
			
			finalRotationMatrix = modelCorrection;
			
		}
		
		
		// Combine transformations in the correct order: Scale -> Rotate -> Translate
		
		auto gizmoModel = actorTranslationMatrix * finalRotationMatrix * scaleMatrix;
		
		
		// Convert the final GLM matrix back to a nanogui matrix and draw
		
		auto gizmoMatrix = glm_to_nanogui(gizmoModel);
		
		auto& drawable = mActiveGizmo->get().get_component<DrawableComponent>();
		
		drawable.draw_content(gizmoMatrix, view, projection);
		
	}
	
}
