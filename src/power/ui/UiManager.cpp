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

class SceneTimeBar : public nanogui::Widget {
public:
	SceneTimeBar(nanogui::Widget* parent, int width, int height) : nanogui::Widget(parent) {
		set_fixed_width(width);
		set_fixed_height(height);
		
		mBackgroundColor = theme()->m_button_gradient_bot_unfocused;
		
		mBackgroundColor.a() = 0.25f;
		
		// Vertical layout for slider above buttons
		set_layout(new nanogui::BoxLayout(nanogui::Orientation::Vertical,
										  nanogui::Alignment::Middle, 0, 0));
		
		// Add the slider above the buttons
		nanogui::Widget* sliderWrapper = new nanogui::Widget(this);
		sliderWrapper->set_layout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal,
														 nanogui::Alignment::Fill, 10, 5));
		
		// Slider for timeline (relative to the SceneTimeBar width)
		nanogui::Slider* timelineSlider = new nanogui::Slider(sliderWrapper);
		timelineSlider->set_value(0.0f);  // Start at 0% of the timeline
		timelineSlider->set_fixed_width(int(this->fixed_width()));
		timelineSlider->set_range(std::make_pair(0.0f, 100.0f));  // Range for timeline
		
		// Create another widget for buttons
		nanogui::Widget* buttonWrapperWrapper = new nanogui::Widget(this);
		

		buttonWrapperWrapper->set_layout(new nanogui::BoxLayout(nanogui::Orientation::Vertical, nanogui::Alignment::Middle, 0, 0));
		
		// Time Counter Display (relative to the SceneTimeBar width)
		mTimeLabel = new nanogui::TextBox(buttonWrapperWrapper);
		mTimeLabel->set_fixed_width(int(this->fixed_width()));  // Set full width for centering
		mTimeLabel->set_font_size(36);
		mTimeLabel->set_alignment(nanogui::TextBox::Alignment::Center);
		
		// Center the text horizontally
		mTimeLabel->set_background_color(nanogui::Color(0, 0, 0, 0));
		
		mTimeLabel->set_value("00:00:00:00");
		
		nanogui::Widget* buttonWrapper = new nanogui::Widget(buttonWrapperWrapper);
		buttonWrapper->set_layout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal, nanogui::Alignment::Middle, 10, 5));
		
		// Rewind Button
		nanogui::Button* rewindBtn = new nanogui::Button(buttonWrapper, "", FA_FAST_BACKWARD);
		rewindBtn->set_fixed_width(int(this->fixed_width() * 0.08));  // 8% of SceneTimeBar width
		rewindBtn->set_fixed_height(int(this->fixed_height() * 0.10));  // 8% of SceneTimeBar height

		// Previous Frame Button
		nanogui::Button* prevFrameBtn = new nanogui::Button(buttonWrapper, "", FA_STEP_BACKWARD);
		prevFrameBtn->set_fixed_width(int(this->fixed_width() * 0.08));  // 8% of SceneTimeBar width
		prevFrameBtn->set_fixed_height(int(this->fixed_height() * 0.10));  // 8% of SceneTimeBar height

		// Play/Pause Button
		nanogui::Button* playPauseBtn = new nanogui::ToolButton(buttonWrapper, FA_PLAY);
		
		auto normalPlayColor = playPauseBtn->text_color();

		playPauseBtn->set_fixed_width(int(this->fixed_width() * 0.08));  // 8% of SceneTimeBar width
		playPauseBtn->set_fixed_height(int(this->fixed_height() * 0.10));  // 8% of SceneTimeBar height

		playPauseBtn->set_change_callback([this, playPauseBtn, normalPlayColor](bool active) {
			toggle_play_pause();
			
			if (active) {
				playPauseBtn->set_icon(FA_PAUSE);
				playPauseBtn->set_text_color(nanogui::Color(0.0f, 0.0f, 1.0f, 1.0f));
			} else {
				playPauseBtn->set_text_color(normalPlayColor);
				playPauseBtn->set_icon(FA_PLAY);
			}

		});
		
		// Record Button
		nanogui::Button* recordBtn = new nanogui::ToolButton(buttonWrapper,  FA_CIRCLE);
		recordBtn->set_fixed_width(int(this->fixed_width() * 0.08));  // 8% of SceneTimeBar width
		recordBtn->set_fixed_height(int(this->fixed_height() * 0.10));  // 8% of SceneTimeBar height
		
		auto normalRecordColor = recordBtn->text_color();
		
		recordBtn->set_change_callback([recordBtn, normalRecordColor](bool active) {
			if (active) {
				recordBtn->set_text_color(nanogui::Color(1.0f, 0.0f, 0.0f, 1.0f));
			} else {
				recordBtn->set_text_color(normalRecordColor);
			}
		});

		// Next Frame Button
		nanogui::Button* nextFrameBtn = new nanogui::Button(buttonWrapper, "", FA_STEP_FORWARD);
		nextFrameBtn->set_fixed_width(int(this->fixed_width() * 0.08));  // 8% of SceneTimeBar width
		nextFrameBtn->set_fixed_height(int(this->fixed_height() * 0.10));  // 8% of SceneTimeBar height

		// Seek to End Button
		nanogui::Button* seekEndBtn = new nanogui::Button(buttonWrapper, "", FA_FAST_FORWARD);
		seekEndBtn->set_fixed_width(int(this->fixed_width() * 0.08));  // 8% of SceneTimeBar width
		seekEndBtn->set_fixed_height(int(this->fixed_height() * 0.10));  // 8% of SceneTimeBar height

	}
	
	bool mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) override {
		Widget::mouse_button_event(p, button, down, modifiers);
		
		return true; // force consume
	}
	
	bool mouse_motion_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override {
		
		Widget::mouse_motion_event(p, rel, button, modifiers);
		
		return true; // force consume
	}
	
	void toggle_play_pause() {
		// Logic to toggle play and pause (to be implemented)
	}
	void update_time_display(int frameCount) {
		const int framesPerSecond = 60; // 60 FPS
		int totalSeconds = frameCount / framesPerSecond;
		int frames = frameCount % framesPerSecond;
		
		int hours = totalSeconds / 3600;
		int minutes = (totalSeconds % 3600) / 60;
		int seconds = totalSeconds % 60;
		
		char buffer[12]; // Adjust buffer size to accommodate frames
		snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d:%02d", hours, minutes, seconds, frames);
		mTimeLabel->set_value(buffer);
	}
	
	virtual void draw(NVGcontext* ctx) override {
		// Draw background
		nvgBeginPath(ctx);
		nvgRect(ctx, m_pos.x(), m_pos.y(), m_size.x(), m_size.y());
		nvgFillColor(ctx, mBackgroundColor); // Set your preferred color (RGBA)
		nvgFill(ctx);
		
		// Call the parent class draw method to render the child widgets
		Widget::draw(ctx);
	}

	
private:
	nanogui::TextBox* mTimeLabel;
	nanogui::Color mBackgroundColor;
};


UiManager::UiManager(IActorSelectedRegistry& registry, IActorVisualManager& actorVisualManager, ActorManager& actorManager, MeshActorLoader& meshActorLoader, ShaderManager& shaderManager, ScenePanel& scenePanel, Canvas& canvas, nanogui::Widget& toolbox, nanogui::Widget& statusBar, CameraManager& cameraManager, std::function<void(std::function<void(int, int)>)> applicationClickRegistrator)
: mRegistry(registry)
, mActorManager(actorManager)
, mShader(*shaderManager.get_shader("mesh"))
, mGizmoManager(std::make_unique<GizmoManager>(toolbox, shaderManager, actorManager))
, mGrid(std::make_unique<Grid>(shaderManager)) {
	mRegistry.RegisterOnActorSelectedCallback(*this);

	// Initialize the scene time bar
	SceneTimeBar* timeBar = new SceneTimeBar(&scenePanel, scenePanel.fixed_width(), scenePanel.fixed_height() * 0.2f);

	// Attach it to the top of the canvas, making it stick at the top
	timeBar->set_position(nanogui::Vector2i(0, 0));  // Stick to top
	timeBar->set_size(nanogui::Vector2i(canvas.width(), 40));  // Full width of the canvas
	

	canvas.register_draw_callback([this, &actorManager, timeBar]() {
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
		}
	});
	
	StatusBarPanel* statusBarPanel = new StatusBarPanel(statusBar, actorVisualManager, meshActorLoader, applicationClickRegistrator);
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
	
	mGizmoManager->select(std::nullopt);
}

void UiManager::draw() {
	mActorManager.visit(*this);
}

void UiManager::draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view,
							 const nanogui::Matrix4f& projection) {
	// Disable writing to the stencil buffer
	glStencilMask(0x00);
	
	mGrid->draw_content(model, view, projection);

	if(mActiveActor.has_value()){
		auto& drawable = mActiveActor->get().get_component<DrawableComponent>();
		auto& color = mActiveActor->get().get_component<ColorComponent>();
		auto& transform = mActiveActor->get().get_component<TransformComponent>();
		
		// Target blue-ish color vector (choose a blend towards red, but not purely red)
		glm::vec3 selection_color(0.83f, 0.68f, 0.21f); // A gold-ish color

		// Normalize the color to ensure its length is exactly 1.0
		selection_color = glm::normalize(selection_color);

		color.apply(selection_color);
		
		nanogui::Matrix4f model = TransformComponent::glm_to_nanogui(transform.get_matrix());

		// Disable stencil test
		glDisable(GL_STENCIL_TEST);
		
		mGrid->draw_content(model, view, projection);

		glClear(GL_DEPTH_BUFFER_BIT);

		glEnable(GL_DEPTH_TEST);
		
		drawable.draw_content(model, view, projection);

		glDisable(GL_DEPTH_TEST);
	}
	
	// Disable stencil test
	glDisable(GL_STENCIL_TEST);
}
 
