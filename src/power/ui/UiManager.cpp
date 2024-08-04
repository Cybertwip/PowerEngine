#include "UiManager.hpp"

#include "Canvas.hpp"

#include "ShaderManager.hpp"

#include "actors/IActorSelectedRegistry.hpp"

#include "actors/Actor.hpp"
#include "actors/ActorManager.hpp"

#include "components/ColorComponent.hpp"
#include "components/DrawableComponent.hpp"
#include "components/MetadataComponent.hpp"
#include "components/TransformComponent.hpp"
#include "components/UiComponent.hpp"

#include "gizmo/GizmoManager.hpp"

#include "ui/ScenePanel.hpp"

#include <nanogui/opengl.h>

UiManager::UiManager(IActorSelectedRegistry& registry, ActorManager& actorManager, ShaderManager& shaderManager, ScenePanel& scenePanel, Canvas& canvas)
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
	
	scenePanel.register_click_callback([this, &canvas](int width, int height, int x, int y){
		
		auto viewport = canvas.render_pass()->viewport();
		
		// Calculate the scaling factors
		float scaleX = viewport.second[0] / float(width);
		float scaleY = viewport.second[1] / float(height);
		
		int adjusted_y = height - y;
		
		// Scale x and y accordingly
		x *= scaleX;
		adjusted_y *= scaleY;

		// Bind the framebuffer from which you want to read pixels
		glBindFramebuffer(GL_READ_FRAMEBUFFER, canvas.render_pass()->framebuffer_handle());
		
		int image_width = 4;
		int image_height = 4;
		
		// Buffer to store the pixel data (16 integers for a 4x4 region)
		std::vector<int> pixels(image_width * image_height);
		
		// Adjust the y-coordinate according to OpenGL's coordinate system
		// Set the read buffer to the appropriate color attachment
		glReadBuffer(GL_COLOR_ATTACHMENT1);
		
		// Read the pixel data from the specified region
		glReadPixels(x, adjusted_y, image_width, image_height, GL_RED_INTEGER, GL_INT, pixels.data());
		
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		
		int id = 0;
		
		for (auto& pixel : pixels) {
			if (pixel != 0) {
				id = pixel;
				break;
			}
		}
		
		if (id != 0) {
			auto actors = mActorManager.get_actors_with_component<MetadataComponent>();
			
			for (auto& actor : actors) {
				auto metadata = actor.get().get_component<MetadataComponent>();
				
				if (id == metadata.identifier()) {
					
					actor.get().get_component<UiComponent>().select();
					OnActorSelected(actor.get());
					break;
				}
			}
		}
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
		
		color.apply(glm::vec3(1.0f, 0.0f, 0.0f));
		
		nanogui::Matrix4f model = TransformComponent::glm_to_nanogui(transform.get_matrix());

		// Second pass: Draw using the stencil buffer as a mask
		glStencilFunc(GL_EQUAL, 1, 0xFF);  // Draw only where stencil value is 1
		glStencilMask(0x00);  // Disable writing to the stencil buffer
		
		glClear(GL_DEPTH_BUFFER_BIT);

		glEnable(GL_DEPTH_TEST);
		
		drawable.draw_content(model, view, projection);
		
		glDisable(GL_DEPTH_TEST);
	}
	
	
	// Disable stencil test
	glDisable(GL_STENCIL_TEST);

}
 
