#include "UiManager.hpp"

#include "Canvas.hpp"

#include "ShaderManager.hpp"

#include "actors/IActorSelectedRegistry.hpp"

#include "actors/Actor.hpp"
#include "actors/ActorManager.hpp"

#include "components/ColorComponent.hpp"
#include "components/DrawableComponent.hpp"
#include "components/TransformComponent.hpp"

#include <nanogui/opengl.h>

UiManager::UiManager(IActorSelectedRegistry& registry, ActorManager& actorManager, ShaderManager& shaderManager, Canvas& canvas)
: mRegistry(registry)
, mActorManager(actorManager)
, mShader(*shaderManager.get_shader("mesh")) {
	mRegistry.RegisterOnActorSelectedCallback(*this);
	
	canvas.register_draw_callback([this]() {
		draw();
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

		
		glDisable(GL_DEPTH_TEST);  // Optionally disable depth test


		drawable.draw_content(model, view, projection);
		
		// Re-enable depth testing after drawing the outlines
		glEnable(GL_DEPTH_TEST);

	}
	
	
	// Disable stencil test
	glDisable(GL_STENCIL_TEST);

}
 
