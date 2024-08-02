#include "UiManager.hpp"

#include "Canvas.hpp"

#include "ShaderManager.hpp"

#include "actors/IActorSelectedRegistry.hpp"

#include "actors/Actor.hpp"
#include "actors/ActorManager.hpp"

#include "components/ColorComponent.hpp"
#include "components/DrawableComponent.hpp"
#include "components/TransformComponent.hpp"

#include "gizmo/GizmoManager.hpp"

#include <nanogui/opengl.h>

UiManager::UiManager(IActorSelectedRegistry& registry, ActorManager& actorManager, ShaderManager& shaderManager, Canvas& canvas)
: mRegistry(registry)
, mActorManager(actorManager)
, mShader(*shaderManager.get_shader("mesh"))
, mGizmoManager(std::make_unique<GizmoManager>(shaderManager, actorManager)) {
	mRegistry.RegisterOnActorSelectedCallback(*this);

	canvas.register_draw_callback([this, &actorManager]() {
		actorManager.draw();
		draw();
		mGizmoManager->draw();
	});
}

UiManager::~UiManager() {
	mRegistry.UnregisterOnActorSelectedCallback(*this);
}

void UiManager::OnActorSelected(Actor& actor) {
	mActiveActor = actor;
}

void UiManager::draw() {
	mActorManager.visit(*this);
}

void UiManager::draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view,
							 const nanogui::Matrix4f& projection) {
		
	// Disable writing to the stencil buffer
	glStencilMask(0x00);

	if(mActiveActor.has_value()){
		auto& drawable = mActiveActor->get().get_component<DrawableComponent>();
		auto& color = mActiveActor->get().get_component<ColorComponent>();
		auto& transform = mActiveActor->get().get_component<TransformComponent>();
		
		color.set_color(glm::vec3(1.0f, 0.0f, 0.0f));
		
		nanogui::Matrix4f model = TransformComponent::glm_to_nanogui(transform.get_matrix());

		// Second pass: Draw using the stencil buffer as a mask
		glStencilFunc(GL_EQUAL, 1, 0xFF);  // Draw only where stencil value is 1
		glStencilMask(0x00);  // Disable writing to the stencil buffer
		
		glClear(GL_DEPTH_BUFFER_BIT);

		glEnable(GL_DEPTH_TEST);  // Optionally disable depth test

		drawable.draw_content(model, view, projection);
		
		// Re-enable depth testing after drawing the outlines
		glDisable(GL_DEPTH_TEST);
	}
	
	
	// Disable stencil test
	glDisable(GL_STENCIL_TEST);

}
 
