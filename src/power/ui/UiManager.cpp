#include "UiManager.hpp"

// ==============================
// Additional Project-Specific Includes
// (Only in Implementation)
// ==============================
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
#include "ui/ResourcesPanel.hpp"
#include "ui/ScenePanel.hpp"
#include "ui/StatusBarPanel.hpp"

// ==============================
// Conditional Compilation
// ==============================
#if defined(NANOGUI_USE_OPENGL) || defined(NANOGUI_USE_GLES)
#include <nanogui/opengl.h>
#elif defined(NANOGUI_USE_METAL)
#include "MetalHelper.hpp"
#endif

#include <nanovg.h>

#include <GLFW/glfw3.h>


// ==============================
// Namespace UIUtils
// ==============================
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

} // namespace UIUtils

// ==============================
// UiManager Constructor
// ==============================
UiManager::UiManager(nanogui::Screen& screen, std::shared_ptr<IActorSelectedRegistry> registry,
					 std::shared_ptr<IActorVisualManager> actorVisualManager,
					 ActorManager& actorManager,
					 MeshActorLoader& meshActorLoader,
					 MeshActorLoader& gizmoActorLoader,
					 ShaderManager& shaderManager,
					 std::shared_ptr<ScenePanel> scenePanel,
					 std::shared_ptr<Canvas> canvas,
					 std::shared_ptr<nanogui::Widget> statusBar,
					 std::shared_ptr<AnimationPanel> animationPanel,
					 AnimationTimeProvider& animationTimeProvider,
					 CameraManager& cameraManager,
					 GizmoManager& gizmoManager,
					 std::function<void(std::function<void(int, int)>)> applicationClickRegistrator)
: mRegistry(registry)
, mActorVisualManager(actorVisualManager)
, mActorManager(actorManager)
, mShaderManager(shaderManager)
, mGrid(std::make_unique<Grid>(shaderManager))
, mMeshActorLoader(meshActorLoader)
, mGizmoActorLoader(gizmoActorLoader)
, mGizmoManager(gizmoManager)
, mCameraManager(cameraManager)
, mCanvas(canvas)
//, mAnimationPanel(animationPanel)
, mIsMovieExporting(false)
, mFrameCounter(0)
, mFramePadding(4) // Initialize with 4 digits (0000)
{
	// Register callbacks
	mRegistry->RegisterOnActorSelectedCallback(*this);
	mRegistry->RegisterOnActorSelectedCallback(mCameraManager);
	
	// Register draw callback with Canvas
	canvas->register_draw_callback([this]() {
		draw();
	});
	
	// Lambda to read from framebuffer
	auto readFromFramebuffer = [this](int width, int height, int x, int y) -> int {
		auto viewport = mCanvas->render_pass().viewport();
		float scaleX = viewport.second[0] / static_cast<float>(width);
		float scaleY = viewport.second[1] / static_cast<float>(height);
		
#if defined(NANOGUI_USE_METAL)
		int adjusted_y = y;
		int adjusted_x = x;
#else
		int adjusted_y = height - y + canvas.parent()->position().y();
		int adjusted_x = x + canvas.parent()->position().x();
#endif
		adjusted_x *= scaleX;
		adjusted_y *= scaleY;
		
		int image_width = 2;
		int image_height = 2;
		std::vector<int> pixels(image_width * image_height, 0);
		
#if defined(NANOGUI_USE_OPENGL) || defined(NANOGUI_USE_GLES)
		glBindFramebuffer(GL_READ_FRAMEBUFFER, canvas.render_pass().framebuffer_handle());
		glReadBuffer(GL_COLOR_ATTACHMENT1);
		glReadPixels(adjusted_x, adjusted_y, image_width, image_height, GL_RED_INTEGER, GL_INT, pixels.data());
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
#elif defined(NANOGUI_USE_METAL)
		auto* attachment_texture = dynamic_cast<nanogui::Texture*>(&mCanvas->render_pass().targets()[3]->get());
		MetalHelper::readPixelsFromMetal(mCanvas->screen().nswin(), attachment_texture->texture_handle(),
										 adjusted_x, adjusted_y, image_width, image_height, pixels);
#endif
		// Find the first non-zero pixel value
		for (const auto& pixel : pixels) {
			if (pixel != 0) {
				return pixel;
			}
		}
		return 0;
	};
	
	// Register click callback with ScenePanel
	scenePanel->register_click_callback(GLFW_MOUSE_BUTTON_1, [this, readFromFramebuffer](bool down, int width, int height, int x, int y) {
		if (down) {
			int id = readFromFramebuffer(width, height, x, y);
			
			if (id != 0) {
				auto actors = mActorManager.get_actors_with_component<ColorComponent>();
				
				for (auto& actor : actors) {
					auto color = actor.get().get_component<ColorComponent>();
					
					if (id == color.identifier()) {
						if (actor.get().find_component<UiComponent>()) {
							actor.get().get_component<UiComponent>().select();
						}
						OnActorSelected(actor.get());
						mGizmoManager.select(mActiveActor);
						break;
					}
				}
				
				mGizmoManager.select(GizmoManager::GizmoAxis(id));
			} else {
				mActiveActor = std::nullopt;
				
				mActorVisualManager->fire_actor_selected_event(mActiveActor);
				mGizmoManager.select(mActiveActor);
				mGizmoManager.select(GizmoManager::GizmoAxis(0));
			}
		} else {
			mGizmoManager.select(GizmoManager::GizmoAxis(0));
			mGizmoManager.hover(GizmoManager::GizmoAxis(0));
		}
	});
	
	// Register motion callback with ScenePanel
	scenePanel->register_motion_callback(GLFW_MOUSE_BUTTON_RIGHT, [this, readFromFramebuffer](int width, int height, int x, int y, int dx, int dy, int button, bool down) {
		
		if (!mCanvas->contains(nanogui::Vector2f(x, y))) {
			return;
		}
		
		if (mActiveActor.has_value()) {
			glm::mat4 viewMatrix = nanogui_to_glm(mCameraManager.get_view());
			glm::mat4 projMatrix = nanogui_to_glm(mCameraManager.get_projection());
			
			auto viewport = mCanvas->render_pass().viewport();
			auto glmViewport = glm::vec4(viewport.first[0], viewport.first[1], viewport.second[0], viewport.second[1]);
			
			auto viewInverse = glm::inverse(viewMatrix);
			glm::vec3 cameraPosition(viewInverse[3][0], viewInverse[3][1], viewInverse[3][2]);
			
			// Calculate the scaling factors
			float scaleX = viewport.second[0] / float(width);
			float scaleY = viewport.second[1] / float(height);
			
			
			int adjusted_y = height - y + mCanvas->parent()->get().position().y();
			int adjusted_x = x + mCanvas->parent()->get().position().x();
			
			int adjusted_dx = x + mCanvas->parent()->get().position().x() + dx;
			int adjusted_dy = height - y + mCanvas->parent()->get().position().y() + dy;
			
			// Scale x and y accordingly
			adjusted_x *= scaleX;
			adjusted_y *= scaleY;
			
			adjusted_dx *= scaleX;
			adjusted_dy *= scaleY;
			
			auto world = UIUtils::ScreenToWorld(glm::vec2(adjusted_x, adjusted_y), cameraPosition.y, projMatrix, viewMatrix, width, height);
			
			auto offset = UIUtils::ScreenToWorld(glm::vec2(adjusted_dx, adjusted_dy), cameraPosition.y, projMatrix, viewMatrix, width, height);
			
			int id = readFromFramebuffer(width, height, x, y);
			
			if (id != 0 && !down) {
				mGizmoManager.hover(GizmoManager::GizmoAxis(id));
			} else if (id == 0 && !down){
				mGizmoManager.hover(GizmoManager::GizmoAxis(0));
			} else if (id != 0 && down) {
				mGizmoManager.hover(GizmoManager::GizmoAxis(id));
			}
			
			// Step 3: Apply the world-space delta transformation
			mGizmoManager.transform(offset.x - world.x, offset.y - world.y);
		}
	});
	
	// Initialize StatusBarPanel
	mStatusBarPanel = std::make_shared<StatusBarPanel>(*statusBar, screen, mActorVisualManager, animationTimeProvider,
													   mMeshActorLoader, mShaderManager, *this, applicationClickRegistrator);
	
	mStatusBarPanel->set_fixed_width(statusBar->fixed_height());
	mStatusBarPanel->set_fixed_height(statusBar->fixed_height());
	mStatusBarPanel->set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Horizontal, nanogui::Alignment::Minimum, 4, 2));
	
	// Initialize selection color
	mSelectionColor = glm::normalize(glm::vec4(0.83f, 0.68f, 0.21f, 1.0f)); // A gold-ish color
	
	scenePanel->register_motion_callback(GLFW_MOUSE_BUTTON_RIGHT, [this](int width, int height, int x, int y, int dx, int dy, int button, bool down){
		// Only rotate camera if no actor is selected (i.e., gizmo is not active)
		if (down && !mActiveActor.has_value()) {
			mCameraManager.rotate_camera(-dx, -dy);
		}
	});
	
	scenePanel->register_motion_callback(GLFW_MOUSE_BUTTON_MIDDLE, [this](int width, int height, int x, int y, int dx, int dy, int button, bool down){
		// Only pan camera if no actor is selected
		if (!mActiveActor.has_value()) {
			mCameraManager.pan_camera(dx, dy);
		}
	});
	
	scenePanel->register_scroll_callback([this](int width, int height, int x, int y, int dx, int dy){
		// Only zoom camera if no actor is selected
		if (!mActiveActor.has_value()) {
			mCameraManager.zoom_camera(-dy);
		}
	});
	
}

// ==============================
// UiManager Destructor
// ==============================
UiManager::~UiManager() {
	mRegistry->UnregisterOnActorSelectedCallback(*this);
	mRegistry->UnregisterOnActorSelectedCallback(mCameraManager);
}

// ==============================
// UiManager Member Functions
// ==============================

void UiManager::OnActorSelected(std::optional<std::reference_wrapper<Actor>> actor) {
	mActiveActor = actor;
	mGizmoManager.select(std::nullopt);
}

void UiManager::export_movie(const std::string& path) {
	mStatusBarPanel->resources_panel()->set_visible(false);
	
	mActiveActor = std::nullopt;
	
	mActorVisualManager->fire_actor_selected_event(mActiveActor);
	mGizmoManager.select(mActiveActor);
	mGizmoManager.select(GizmoManager::GizmoAxis(0));
	
	//	mSceneTimeBar->stop_playback();
	//	mSceneTimeBar->toggle_play_pause(true);
	
	nanogui::async([this, path]() {
		mIsMovieExporting = true;
		
		mMovieExportFile = path;
		mMovieExportDirectory = std::filesystem::path(path).parent_path().string();
	});
}

bool UiManager::is_movie_exporting() const {
	return mIsMovieExporting;
}

void UiManager::draw() {
	if (mIsMovieExporting) {
		//		mSceneTimeBar->update();
		// Begin the first render pass for actors
		mCanvas->render_pass().clear_color(0, mCanvas->background_color());
		mCanvas->render_pass().clear_color(1, nanogui::Color(0.0f, 0.0f, 0.0f, 0.0f));
		mCanvas->render_pass().clear_depth(1.0f);
		
		// Draw all actors
		mActorManager.draw();
		
		mCanvas->render_pass().set_depth_test(nanogui::RenderPass::DepthTest::Less, true);
		
		auto& batch_unit = mMeshActorLoader.get_batch_unit();
		mActorManager.visit(batch_unit.mMeshBatch);
		mActorManager.visit(batch_unit.mSkinnedMeshBatch);
		
		// Take snapshot for movie export
		mCanvas->take_snapshot([this](std::vector<uint8_t>& png_data) {
			// Generate the frame filename with padded zeros
			std::ostringstream filename_stream;
			filename_stream << "frame" << std::setw(mFramePadding) << std::setfill('0') << mFrameCounter << ".jpg";
			std::string filename = filename_stream.str();
			
			// Full path for the frame
			std::filesystem::path frame_path = std::filesystem::path(mMovieExportDirectory) / filename;
			
			// Write the PNG data to the file
			std::ofstream file(frame_path, std::ios::binary);
			if (file.is_open()) {
				file.write(reinterpret_cast<const char*>(png_data.data()), png_data.size());
				file.close();
				std::cout << "Saved frame: " << frame_path << std::endl;
			} else {
				std::cerr << "Failed to save frame: " << frame_path << std::endl;
			}
			
			mFrameCounter++;
			
			// Check if padding needs to be increased
			int max_frames = static_cast<int>(std::pow(10, mFramePadding) - 1);
			if (mFrameCounter > max_frames) {
				mFramePadding += 1; // Increase padding by 1 digit
				std::cout << "Increased frame padding to: " << mFramePadding << " digits." << std::endl;
			}
		});
		
		//		if (!mSceneTimeBar->is_playing()) {
		//			mIsMovieExporting = false;
		//
		//			// Assemble the movie using ffmpeg
		//			std::ostringstream ffmpeg_command;
		//			ffmpeg_command << "/usr/local/bin/ffmpeg -y -framerate 60 -i \""
		//			<< mMovieExportDirectory << "/frame%0" << mFramePadding
		//			<< "d.jpg\" -c:v libx264 -pix_fmt yuv420p \""
		//			<< mMovieExportFile << "\"";
		//
		//			std::cout << "Running ffmpeg command: " << ffmpeg_command.str() << std::endl;
		//
		//			int ret = std::system(ffmpeg_command.str().c_str());
		//			if (ret != 0) {
		//				std::cerr << "ffmpeg failed with return code: " << ret << std::endl;
		//			} else {
		//				std::cout << "Movie exported successfully to: " << mMovieExportFile << std::endl;
		//			}
		//
		//			// Optional: Clean up frame images
		//			for (int i = 0; i < mFrameCounter; ++i) {
		//				std::ostringstream cleanup_stream;
		//				cleanup_stream << "frame" << std::setw(mFramePadding) << std::setfill('0') << i << ".jpg";
		//				std::string cleanup_filename = cleanup_stream.str();
		//				std::filesystem::path cleanup_path = std::filesystem::path(mMovieExportDirectory) / cleanup_filename;
		//				if (std::remove(cleanup_path.string().c_str()) != 0) {
		//					std::cerr << "Failed to delete frame: " << cleanup_path << std::endl;
		//				}
		//			}
		//		}
	} else {
		// Update UI elements
		//		mSceneTimeBar->update();
		//		mSceneTimeBar->overlay();
		//		mAnimationPanel->update_with(mSceneTimeBar->current_time());
		
		// Begin the first render pass for main actors
		mCanvas->render_pass().clear_color(0, mCanvas->background_color());
		mCanvas->render_pass().clear_color(1, nanogui::Color(0.0f, 0.0f, 0.0f, 0.0f));
		mCanvas->render_pass().clear_depth(1.0f);
		mCanvas->render_pass().set_depth_test(nanogui::RenderPass::DepthTest::Less, true);
		
		// Draw all actors
		mActorManager.draw();
		
		if (mActiveActor) {
			if (mActiveActor->get().find_component<ColorComponent>()) {
				auto& color = mActiveActor->get().get_component<ColorComponent>();
				color.set_color(mSelectionColor);
			}
		}
		
		auto& batch_unit = mMeshActorLoader.get_batch_unit();
		mActorManager.visit(batch_unit.mMeshBatch);
		mActorManager.visit(batch_unit.mSkinnedMeshBatch);
		
		mActorManager.visit(*this);
		
		mCanvas->render_pass().set_depth_test(nanogui::RenderPass::DepthTest::Less, true);
		
		// Draw gizmos
		mGizmoManager.draw();
		auto& gizmo_batch_unit = mGizmoActorLoader.get_batch_unit();
		mActorManager.visit(gizmo_batch_unit.mMeshBatch);
		mActorManager.visit(gizmo_batch_unit.mSkinnedMeshBatch);
	}
	
}

void UiManager::draw_content(const nanogui::Matrix4f& model,
							 const nanogui::Matrix4f& view,
							 const nanogui::Matrix4f& projection) {
	// Draw the grid first
	mGrid->draw_content(model, view, projection);
}

void UiManager::remove_active_actor() {
	std::async([this]() {
		if (mActiveActor.has_value()) {
			mActorVisualManager->remove_actor(mActiveActor->get());
			//			mSceneTimeBar->refresh_actors();
		}
	});
}

void UiManager::process_events() {
	mCanvas->process_events();
}

std::shared_ptr<StatusBarPanel> UiManager::status_bar_panel() {
	return mStatusBarPanel;
}
