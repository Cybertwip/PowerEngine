#include "UiManager.hpp"

#include "Canvas.hpp"
#include "CameraManager.hpp"

#include "MeshActorLoader.hpp"

#include "ShaderManager.hpp"

#include "actors/IActorSelectedRegistry.hpp"

#include "actors/Actor.hpp"
#include "actors/ActorManager.hpp"

#include "animation/Animation.hpp"
#include "animation/Skeleton.hpp"

#include "components/AnimationComponent.hpp"
#include "components/ColorComponent.hpp"
#include "components/DrawableComponent.hpp"
#include "components/MetadataComponent.hpp"
#include "components/PlaybackComponent.hpp"
#include "components/TransformComponent.hpp"
#include "components/UiComponent.hpp"

#include "gizmo/GizmoManager.hpp"

#include "graphics/drawing/BatchUnit.hpp"
#include "graphics/drawing/MeshBatch.hpp"
#include "graphics/drawing/SkinnedMeshBatch.hpp"
#include "graphics/drawing/Grid.hpp"

#include "ui/AnimationPanel.hpp"
#include "ui/ScenePanel.hpp"
#include "ui/SceneTimeBar.hpp"
#include "ui/StatusBarPanel.hpp"

#if defined(NANOGUI_USE_OPENGL) || defined(NANOGUI_USE_GLES)

#include <nanogui/opengl.h>

#elif defined(NANOGUI_USE_METAL)

#include "MetalHelper.hpp"

#endif

#include <nanovg.h>


#include <iostream>

namespace UIUtils {

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

UiManager::UiManager(IActorSelectedRegistry& registry, IActorVisualManager& actorVisualManager, ActorManager& actorManager, MeshActorLoader& meshActorLoader, ShaderManager& shaderManager, ScenePanel& scenePanel, Canvas& canvas, nanogui::Widget& toolbox, nanogui::Widget& statusBar, AnimationPanel& animationPanel, SceneTimeBar& sceneTimeBar, CameraManager& cameraManager, DeepMotionApiClient& deepMotionApiClient, std::function<void(std::function<void(int, int)>)> applicationClickRegistrator)
: mRegistry(registry)
, mActorManager(actorManager)
, mShaderManager(shaderManager)
, mGrid(std::make_unique<Grid>(shaderManager))
, mMeshActorLoader(meshActorLoader)
, mGizmoManager(std::make_unique<GizmoManager>(toolbox, shaderManager, actorManager, mMeshActorLoader))

, mCanvas(canvas)
, mAnimationPanel(animationPanel)
, mSceneTimeBar(sceneTimeBar)
, mIsMovieExporting(false) {
	//
	//	mRenderPass = new nanogui::RenderPass({mCanvas.render_pass()->targets()[2],
	//		mCanvas.render_pass()->targets()[3]}, mCanvas.render_pass()->targets()[0], mCanvas.render_pass()->targets()[1], nullptr);
	
	mRegistry.RegisterOnActorSelectedCallback(*this);
		
	canvas.register_draw_callback([this]() {
		draw();
	});
	
	auto readFromFramebuffer = [&canvas](int width, int height, int x, int y){
		auto viewport = canvas.render_pass()->viewport();
		
		// Calculate the scaling factors
		float scaleX = viewport.second[0] / float(width);
		float scaleY = viewport.second[1] / float(height);
		
#if defined(NANOGUI_USE_METAL)
		int adjusted_y = y;
		int adjusted_x = x;
#else
		int adjusted_y = height - y + canvas.parent()->position().y();
		int adjusted_x = x + canvas.parent()->position().x();
#endif
		
		// Scale x and y accordingly
		adjusted_x *= scaleX;
		adjusted_y *= scaleY;
		
		int image_width = 2;
		int image_height = 2;
		
		std::vector<int> pixels;
		
		pixels.resize(image_width * image_height);
		
#if defined(NANOGUI_USE_OPENGL) || defined(NANOGUI_USE_GLES)
		// Bind the framebuffer from which you want to read pixels (OpenGL/GLES)
		glBindFramebuffer(GL_READ_FRAMEBUFFER, canvas.render_pass()->framebuffer_handle());
		
		// Adjust the y-coordinate according to OpenGL's coordinate system
		// Set the read buffer to the appropriate color attachment
		glReadBuffer(GL_COLOR_ATTACHMENT1);
		
		// Read the pixel data from the specified region (OpenGL)
		glReadPixels(adjusted_x, adjusted_y, image_width, image_height, GL_RED_INTEGER, GL_INT, pixels.data());
		
		// Restore the default buffer and unbind the framebuffer
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		
#elif defined(NANOGUI_USE_METAL)
		nanogui::Texture *attachment_texture =
		dynamic_cast<nanogui::Texture *>(canvas.render_pass()->targets()[3]);
		
		// Metal specific code using MetalHelper
		MetalHelper::readPixelsFromMetal(canvas.screen()->nswin(), attachment_texture->texture_handle(), adjusted_x, adjusted_y, image_width, image_height, pixels);
#endif
		
		int id = 0;
		
		// Find the first non-zero pixel value
		for (auto& pixel : pixels) {
			if (pixel != 0) {
				id = pixel;
				break;
			}
		}
		
		return id;
	};
	
	
	scenePanel.register_click_callback([this, &canvas, &toolbox, readFromFramebuffer, &actorVisualManager](bool down, int width, int height, int x, int y){
		
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
						if (actor.get().find_component<UiComponent>()) {
							actor.get().get_component<UiComponent>().select();
						}
						OnActorSelected(actor.get());
						mGizmoManager->select(mActiveActor);
						break;
					}
				}
				
				mGizmoManager->select(GizmoManager::GizmoAxis(id));
			} else {
				mActiveActor = std::nullopt;
				
				actorVisualManager.fire_actor_selected_event(mActiveActor);
				mGizmoManager->select(mActiveActor);
				mGizmoManager->select(GizmoManager::GizmoAxis(0));
			}
		} else {
			mGizmoManager->select(GizmoManager::GizmoAxis(0));
			mGizmoManager->hover(GizmoManager::GizmoAxis(0));
		}
	});
	
	scenePanel.register_motion_callback([this, &canvas, &toolbox, &cameraManager, readFromFramebuffer](int width, int height, int x, int y, int dx, int dy, int button, bool down){
		
		if (toolbox.contains(nanogui::Vector2f(x, y)) || !canvas.contains(nanogui::Vector2f(x, y))) {
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
			
			auto world = UIUtils::ScreenToWorld(glm::vec2(adjusted_x, adjusted_y), cameraPosition.z, projMatrix, viewMatrix, width, height);
			
			auto offset = UIUtils::ScreenToWorld(glm::vec2(adjusted_dx, adjusted_dy), cameraPosition.z, projMatrix, viewMatrix, width, height);
			
			int id = readFromFramebuffer(width, height, x, y);
			
			if (id != 0 && !down) {
				mGizmoManager->hover(GizmoManager::GizmoAxis(id));
			} else if (id == 0 && !down){
				mGizmoManager->hover(GizmoManager::GizmoAxis(0));
			} else if (id != 0 && down) {
				mGizmoManager->hover(GizmoManager::GizmoAxis(id));
			}
			
			// Step 3: Apply the world-space delta transformation
			mGizmoManager->transform(offset.x - world.x, offset.y - world.y);
		}
	});
	
	mStatusBarPanel = new StatusBarPanel(statusBar, actorVisualManager, mSceneTimeBar, meshActorLoader, shaderManager, deepMotionApiClient, applicationClickRegistrator);
	mStatusBarPanel->set_fixed_width(statusBar.fixed_height());
	mStatusBarPanel->set_fixed_height(statusBar.fixed_height());
	mStatusBarPanel->set_layout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal,
													   nanogui::Alignment::Minimum, 4, 2));
	
	mSelectionColor = glm::vec4(0.83f, 0.68f, 0.21f, 1.0f); // A gold-ish color
	
	mSelectionColor = glm::normalize(mSelectionColor);
}

UiManager::~UiManager() {
	mRegistry.UnregisterOnActorSelectedCallback(*this);
}

void UiManager::OnActorSelected(std::optional<std::reference_wrapper<Actor>> actor) {
	mActiveActor = actor;
	
	mGizmoManager->select(std::nullopt);
}

void UiManager::export_movie(const std::string& path) {
	mSceneTimeBar.stop_playback();
	mSceneTimeBar.toggle_play_pause(true);
	
	mIsMovieExporting = true;
}

void UiManager::draw() {
	if (mIsMovieExporting) {
		mSceneTimeBar.update();
		// Begin the first render pass for actors
		mCanvas.render_pass()->clear_color(0, mCanvas.background_color());
		mCanvas.render_pass()->clear_color(1, nanogui::Color(0.0f, 0.0f, 0.0f, 0.0f));
		mCanvas.render_pass()->clear_depth(1.0f);
		
		// Draw all actors
		mActorManager.draw();
		
		// One for each batch
		mCanvas.render_pass()->push_depth_test_state(nanogui::RenderPass::DepthTest::Less, true, mShaderManager.identifier("mesh"));
		
		mCanvas.render_pass()->push_depth_test_state(nanogui::RenderPass::DepthTest::Less, true, mShaderManager.identifier("skinned_mesh"));
		
		auto& batch_unit = mMeshActorLoader.get_batch_unit();
		
		mActorManager.visit(batch_unit.mMeshBatch);
		
		mActorManager.visit(batch_unit.mSkinnedMeshBatch);
		
		mCanvas.take_snapshot([this](std::vector<uint8_t>& pixels) {
			
		});
	} else {
		mSceneTimeBar.update();
		mSceneTimeBar.overlay();

		mAnimationPanel.update_with(mSceneTimeBar.current_time());
		
		// Begin the first render pass for actors
		mCanvas.render_pass()->clear_color(0, mCanvas.background_color());
		mCanvas.render_pass()->clear_color(1, nanogui::Color(0.0f, 0.0f, 0.0f, 0.0f));
		mCanvas.render_pass()->clear_depth(1.0f);
		
		// Draw all actors
		mActorManager.draw();
		
		if (mActiveActor) {
			auto& color = mActiveActor->get().get_component<ColorComponent>();
			color.set_color(mSelectionColor);
		}
		
		mCanvas.render_pass()->push_depth_test_state(nanogui::RenderPass::DepthTest::Always, true, mShaderManager.identifier("gizmo"));
		
		mCanvas.render_pass()->push_depth_test_state(nanogui::RenderPass::DepthTest::Always, true, mShaderManager.identifier("gizmo"));
		
		mCanvas.render_pass()->push_depth_test_state(nanogui::RenderPass::DepthTest::Always, true, mShaderManager.identifier("gizmo"));
		
		// Draw gizmos
		mGizmoManager->draw();
		
		// One for each batch
		mCanvas.render_pass()->push_depth_test_state(nanogui::RenderPass::DepthTest::Less, true, mShaderManager.identifier("mesh"));
		
		mCanvas.render_pass()->push_depth_test_state(nanogui::RenderPass::DepthTest::Less, true, mShaderManager.identifier("skinned_mesh"));
		
		auto& batch_unit = mMeshActorLoader.get_batch_unit();
		
		mActorManager.visit(batch_unit.mMeshBatch);
		
		mActorManager.visit(batch_unit.mSkinnedMeshBatch);
		
		mCanvas.render_pass()->set_depth_test(nanogui::RenderPass::DepthTest::Less, true);
		
		mActorManager.visit(*this);
	}

}

void UiManager::draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view,
							 const nanogui::Matrix4f& projection) {
	// Draw the grid first
	mGrid->draw_content(model, view, projection);
}

void UiManager::process_events() {
	mCanvas.process_events();
}
