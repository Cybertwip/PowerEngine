#include "UiManager.hpp"

#include "Canvas.hpp"
#include "CameraManager.hpp"

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
#include "gizmo/TranslationGizmo.hpp"
#include "gizmo/RotationGizmo.hpp"
#include "gizmo/ScaleGizmo.hpp"

#include "graphics/drawing/Grid.hpp"

#include "ui/ScenePanel.hpp"
#include "ui/StatusBarPanel.hpp"

#include <nanogui/opengl.h>

#include <iostream>

namespace {

glm::vec3 ScreenToWorld(glm::vec2 screenPos, float depth, glm::mat4 projectionMatrix, glm::mat4 viewMatrix, int screenWidth, int screenHeight) {
	glm::mat4 Projection = projectionMatrix;
	glm::mat4 ProjectionInv = glm::inverse(Projection);
	
	int WINDOW_WIDTH = screenWidth;
	int WINDOW_HEIGHT = screenHeight;
	
	// Step 1 - Viewport to NDC
	float mouse_x = screenPos.x;
	float mouse_y = screenPos.y;
	
	float ndc_x = (2.0f * mouse_x) / WINDOW_WIDTH - 1.0f;
	float ndc_y = 1.0f - (2.0f * mouse_y) / WINDOW_HEIGHT; // flip the Y axis
	
	// Step 2 - NDC to view (my version)
	float focal_length = 1.0f/tanf(glm::radians(45.0f / 2.0f));
	float ar = (float)WINDOW_HEIGHT / (float)WINDOW_WIDTH;
	glm::vec3 ray_view(ndc_x / focal_length, (ndc_y * ar) / focal_length, 1.0f);
	
	// Step 2 - NDC to view (Anton's version)
	glm::vec4 ray_ndc_4d(ndc_x, ndc_y, 1.0f, 1.0f);
	glm::vec4 ray_view_4d = ProjectionInv * ray_ndc_4d;
	
	// Step 3 - intersect view vector with object Z plane (in view)
	glm::vec4 view_space_intersect = glm::vec4(ray_view * depth, 1.0f);
	
	// Step 4 - View to World space
	glm::mat4 View = viewMatrix;
	glm::mat4 InvView = glm::inverse(viewMatrix);
	glm::vec4 point_world = InvView * view_space_intersect;
	return glm::vec3(point_world);
}

}

UiManager::UiManager(IActorSelectedRegistry& registry, IActorVisualManager& actorVisualManager, ActorManager& actorManager, MeshActorLoader& meshActorLoader, ShaderManager& shaderManager, ScenePanel& scenePanel, Canvas& canvas, nanogui::Widget& toolbox, nanogui::Widget& statusBar, CameraManager& cameraManager)
: mRegistry(registry)
, mActorManager(actorManager)
, mShader(*shaderManager.get_shader("mesh"))
, mGizmoManager(std::make_unique<GizmoManager>(toolbox, shaderManager, actorManager))
, mGrid(std::make_unique<Grid>(shaderManager)) {
	mRegistry.RegisterOnActorSelectedCallback(*this);

	canvas.register_draw_callback([this, &actorManager]() {
		actorManager.draw();
		draw();
		mGizmoManager->draw();
	});
	
	auto readFromFramebuffer = [&canvas](int width, int height, int x, int y){
		auto viewport = canvas.render_pass()->viewport();
		
		// Calculate the scaling factors
		float scaleX = viewport.second[0] / float(width);
		float scaleY = viewport.second[1] / float(height);
		
		int adjusted_y = height - y + canvas.parent()->position().y();
		int adjusted_x = x + canvas.parent()->position().x();
		
		// Scale x and y accordingly
		adjusted_x *= scaleX;
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
		glReadPixels(adjusted_x, adjusted_y, image_width, image_height, GL_RED_INTEGER, GL_INT, pixels.data());
		
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		
		int id = 0;
		
		for (auto& pixel : pixels) {
			if (pixel != 0) {
				id = pixel;
				break;
			}
		}

		return id;
	};
	
	scenePanel.register_click_callback([this, &canvas, &toolbox, readFromFramebuffer](bool down, int width, int height, int x, int y){
		
		if (toolbox.contains(nanogui::Vector2f(x, y))) {
			return;
		}
		
		if (down) {
			int id = readFromFramebuffer(width, height, x, y);
			
			if (id != 0) {
				auto actors = mActorManager.get_actors_with_component<MetadataComponent>();
				
				for (auto& actor : actors) {
					auto metadata = actor.get().get_component<MetadataComponent>();
					
					if (id == metadata.identifier()) {
						actor.get().get_component<UiComponent>().select();
						OnActorSelected(actor.get());
						mGizmoManager->select(mActiveActor);
						break;
					}
				}
				
				mGizmoManager->select(id);
			} else {
				mActiveActor = std::nullopt;
				mGizmoManager->select(mActiveActor);
				mGizmoManager->select(0);
			}
		} else {
			mGizmoManager->select(0);
			mGizmoManager->hover(0);
		}
	});
	
	scenePanel.register_motion_callback([this, &canvas, &toolbox, &cameraManager, readFromFramebuffer](int width, int height, int x, int y, int dx, int dy, int button, bool down){
		
		if (toolbox.contains(nanogui::Vector2f(x, y))) {
			return;
		}

		if (mActiveActor.has_value()) {
			glm::mat4 viewMatrix = TransformComponent::nanogui_to_glm(cameraManager.get_view());
			glm::mat4 projMatrix = TransformComponent::nanogui_to_glm(cameraManager.get_projection());
			
			auto viewport = canvas.render_pass()->viewport();
			auto glmViewport = glm::vec4(viewport.first[0], viewport.first[1], viewport.second[0], viewport.second[1]);
			
			auto viewInverse = glm::inverse(viewMatrix);
			glm::vec3 cameraPosition(viewInverse[3][0], viewInverse[3][1], viewInverse[3][2]);
			
			// Calculate the scaling factors
			float scaleX = viewport.second[0] / float(width);
			float scaleY = viewport.second[1] / float(height);


			int adjusted_y = height - y + canvas.parent()->position().y();
			int adjusted_x = x + canvas.parent()->position().x();

			int adjusted_dx = x + canvas.parent()->position().x() + dx;
			int adjusted_dy = height - y + canvas.parent()->position().y() + dy;

			// Scale x and y accordingly
			adjusted_x *= scaleX;
			adjusted_y *= scaleY;

			adjusted_dx *= scaleX;
			adjusted_dy *= scaleY;

			auto world = ScreenToWorld(glm::vec2(adjusted_x, adjusted_y), cameraPosition.z, projMatrix, viewMatrix, width, height);

			auto offset = ScreenToWorld(glm::vec2(adjusted_dx, adjusted_dy), cameraPosition.z, projMatrix, viewMatrix, width, height);

			int id = readFromFramebuffer(width, height, x, y);
			
			if (id != 0 && !down) {
				mGizmoManager->hover(id);
			} else if (id == 0 && !down){
				mGizmoManager->hover(0);
			} else if (id != 0 && down) {
				mGizmoManager->hover(id);
			}
			
			// Step 3: Apply the world-space delta transformation
			mGizmoManager->transform(world.x - offset.x, world.y - offset.y);

			mActiveActor->get().get_component<UiComponent>().select();
		}
	});
	
	StatusBarPanel* statusBarPanel = new StatusBarPanel(statusBar, actorVisualManager, meshActorLoader);
	statusBarPanel->set_fixed_width(statusBar.fixed_height());
	statusBarPanel->set_fixed_height(statusBar.fixed_height());
	statusBarPanel->set_layout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal,
													   nanogui::Alignment::Minimum, 4, 2));
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
	
	mGrid->draw_content(model, view, projection);
}
 
