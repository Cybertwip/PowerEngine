#include "UiManager.hpp"

#include "Canvas.hpp"
#include "CameraManager.hpp"

#include "ShaderManager.hpp"

#include "actors/IActorSelectedRegistry.hpp"

#include "actors/Actor.hpp"
#include "actors/ActorManager.hpp"

#include "components/AnimationComponent.hpp"
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


class SceneTimeBar : public nanogui::Widget, public IActorSelectedCallback {
public:
	SceneTimeBar(nanogui::Widget* parent, ActorManager& actorManager, int width, int height)
	: nanogui::Widget(parent),
	mActorManager(actorManager),
	mTransformRegistrationId(-1),
	mCurrentTime(0),
	mTotalFrames(1800), // 30 seconds at 60 FPS
	mRecording(false),
	mPlaying(false) {
		
		// Set fixed dimensions
		set_fixed_width(width);
		set_fixed_height(height);
		
		mBackgroundColor = theme()->m_button_gradient_bot_unfocused;
		mBackgroundColor.a() = 0.25f;
		
		// Cache fixed dimensions
		int fixedWidth = fixed_width();
		int fixedHeight = fixed_height();
		int buttonWidth = static_cast<int>(fixedWidth * 0.08f);
		int buttonHeight = static_cast<int>(fixedHeight * 0.10f);
		
		// Vertical layout for slider above buttons
		set_layout(new nanogui::BoxLayout(nanogui::Orientation::Vertical, nanogui::Alignment::Middle, 0, 0));
		
		// Slider Wrapper
		nanogui::Widget* sliderWrapper = new nanogui::Widget(this);
		sliderWrapper->set_layout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal, nanogui::Alignment::Fill, 10, 5));
		
		// Timeline Slider
		mTimelineSlider = new nanogui::Slider(sliderWrapper);
		mTimelineSlider->set_value(0.0f);  // Start at 0%
		mTimelineSlider->set_fixed_width(fixedWidth);
		mTimelineSlider->set_range(std::make_pair(0.0f, 1.0f));  // Normalized range
		
		// Slider Callback to update mCurrentTime
		mTimelineSlider->set_callback([this](float value) {
			mCurrentTime = static_cast<int>(value * mTotalFrames);
			update_time_display(mCurrentTime);
			// Optionally, evaluate animations immediately when scrubbing
			evaluate_animations();
		});
		
		// Buttons Wrapper
		nanogui::Widget* buttonWrapperWrapper = new nanogui::Widget(this);
		buttonWrapperWrapper->set_layout(new nanogui::BoxLayout(nanogui::Orientation::Vertical, nanogui::Alignment::Middle, 0, 0));
		
		// Time Counter Display
		mTimeLabel = new nanogui::TextBox(buttonWrapperWrapper);
		mTimeLabel->set_fixed_width(fixedWidth);
		mTimeLabel->set_font_size(36);
		mTimeLabel->set_alignment(nanogui::TextBox::Alignment::Center);
		mTimeLabel->set_background_color(nanogui::Color(0, 0, 0, 0));
		mTimeLabel->set_value("00:00:00:00");
		
		// Buttons Horizontal Layout
		nanogui::Widget* buttonWrapper = new nanogui::Widget(buttonWrapperWrapper);
		buttonWrapper->set_layout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal, nanogui::Alignment::Middle, 10, 5));
		
		// Rewind Button
		nanogui::Button* rewindBtn = new nanogui::Button(buttonWrapper, "", FA_FAST_BACKWARD);
		rewindBtn->set_fixed_width(buttonWidth);
		rewindBtn->set_fixed_height(buttonHeight);
		rewindBtn->set_tooltip("Rewind to Start");
		rewindBtn->set_callback([this]() {
			stop_playback(); // Ensure playback is stopped
			mCurrentTime = 0;
			update_time_display(mCurrentTime);
			mTimelineSlider->set_value(0.0f);
			evaluate_animations();
		});
		
		// Previous Frame Button
		nanogui::Button* prevFrameBtn = new nanogui::Button(buttonWrapper, "", FA_STEP_BACKWARD);
		prevFrameBtn->set_fixed_width(buttonWidth);
		prevFrameBtn->set_fixed_height(buttonHeight);
		prevFrameBtn->set_tooltip("Previous Frame");
		prevFrameBtn->set_callback([this]() {
			if (mCurrentTime > 0) {
				mCurrentTime--;
				update_time_display(mCurrentTime);
				mTimelineSlider->set_value(static_cast<float>(mCurrentTime) / mTotalFrames);
				evaluate_animations();
			}
		});
		
		// Play/Pause Button
		mPlayPauseBtn = new nanogui::ToolButton(buttonWrapper, FA_PLAY);
		mPlayPauseBtn->set_fixed_width(buttonWidth);
		mPlayPauseBtn->set_fixed_height(buttonHeight);
		mPlayPauseBtn->set_tooltip("Play");
		mPlayPauseBtn->set_flags(nanogui::Button::ToggleButton);
		auto normalPlayColor = mPlayPauseBtn->text_color();
		
		mPlayPauseBtn->set_change_callback([this, normalPlayColor](bool active) {
			if (active) {
				// Play
				toggle_play_pause(true);
				mPlayPauseBtn->set_icon(FA_PAUSE);
				mPlayPauseBtn->set_tooltip("Pause");
				mPlayPauseBtn->set_text_color(nanogui::Color(0.0f, 0.0f, 1.0f, 1.0f)); // Blue
			} else {
				// Pause
				toggle_play_pause(false);
				mPlayPauseBtn->set_icon(FA_PLAY);
				mPlayPauseBtn->set_tooltip("Play");
				mPlayPauseBtn->set_text_color(normalPlayColor);
			}
			
			mRecordBtn->set_text_color(normalPlayColor);
		});
		
		// Stop Button
		nanogui::Button* stopBtn = new nanogui::Button(buttonWrapper, "", FA_STOP);
		stopBtn->set_fixed_width(buttonWidth);
		stopBtn->set_fixed_height(buttonHeight);
		stopBtn->set_tooltip("Stop");
		stopBtn->set_callback([this]() {
			stop_playback();
			mCurrentTime = 0;
			update_time_display(mCurrentTime);
			mTimelineSlider->set_value(0.0f);
			evaluate_animations();
		});
		
		// Record Button
		mRecordBtn = new nanogui::ToolButton(buttonWrapper, FA_CIRCLE);
		mRecordBtn->set_fixed_width(buttonWidth);
		mRecordBtn->set_fixed_height(buttonHeight);
		mRecordBtn->set_tooltip("Record");
		mRecordBtn->set_flags(nanogui::Button::ToggleButton);
		auto normalRecordColor = mRecordBtn->text_color();
		
		mRecordBtn->set_change_callback([this, normalRecordColor](bool active) {
			if (!mPlaying) {
				mRecording = active;
				mRecordBtn->set_text_color(active ? nanogui::Color(1.0f, 0.0f, 0.0f, 1.0f) : normalRecordColor); // Red when recording
			}
		});
		
		// Next Frame Button
		nanogui::Button* nextFrameBtn = new nanogui::Button(buttonWrapper, "", FA_STEP_FORWARD);
		nextFrameBtn->set_fixed_width(buttonWidth);
		nextFrameBtn->set_fixed_height(buttonHeight);
		nextFrameBtn->set_tooltip("Next Frame");
		nextFrameBtn->set_callback([this]() {
			if (mCurrentTime < mTotalFrames) {
				mCurrentTime++;
				update_time_display(mCurrentTime);
				mTimelineSlider->set_value(static_cast<float>(mCurrentTime) / mTotalFrames);
				evaluate_animations();
			}
		});
		
		// Seek to End Button
		nanogui::Button* seekEndBtn = new nanogui::Button(buttonWrapper, "", FA_FAST_FORWARD);
		seekEndBtn->set_fixed_width(buttonWidth);
		seekEndBtn->set_fixed_height(buttonHeight);
		seekEndBtn->set_tooltip("Seek to End");
		seekEndBtn->set_callback([this]() {
			stop_playback(); // Ensure playback is stopped
			mCurrentTime = mTotalFrames;
			update_time_display(mCurrentTime);
			mTimelineSlider->set_value(1.0f);
			evaluate_animations();
		});
	}
	
	// Override OnActorSelected from IActorSelectedCallback
	void OnActorSelected(Actor& actor) override {
		if (mActiveActor.has_value()) {
			auto& transformComponent = mActiveActor->get().get_component<TransformComponent>();
			transformComponent.unregister_on_transform_changed_callback(mTransformRegistrationId);
		}
		
		mActiveActor = actor;
		
		auto& transformComponent = mActiveActor->get().get_component<TransformComponent>();
		
		auto& animationComponent = mActiveActor->get().get_component<AnimationComponent>();
		
		mTransformRegistrationId = transformComponent.register_on_transform_changed_callback([this, &animationComponent](const TransformComponent& transform) {
			if (mRecording) {
				animationComponent.addKeyframe(mCurrentTime, transform.get_translation(), transform.get_rotation(), transform.get_scale());
			}
		});
	}
	
	// Override mouse events to consume them
	bool mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) override {
		nanogui::Widget::mouse_button_event(p, button, down, modifiers);
		return true; // Consume the event
	}
	
	bool mouse_motion_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override {
		nanogui::Widget::mouse_motion_event(p, rel, button, modifiers);
		return true; // Consume the event
	}
	
	// Override the draw method to handle time updates and rendering
	virtual void draw(NVGcontext* ctx) override {
		if (mPlaying) {
			if (mCurrentTime < mTotalFrames) {
				mCurrentTime++;
				update_time_display(mCurrentTime);
				mTimelineSlider->set_value(static_cast<float>(mCurrentTime) / mTotalFrames);
			} else {
				// Stop playing when end is reached
				stop_playback();
			}
		}
		
		evaluate_animations();

		// Draw background
		nvgBeginPath(ctx);
		nvgRect(ctx, m_pos.x(), m_pos.y(), m_size.x(), m_size.y());
		nvgFillColor(ctx, mBackgroundColor);
		nvgFill(ctx);
		
		// Call the parent class draw method to render the child widgets
		nanogui::Widget::draw(ctx);
	}
	
private:
	// Helper method to toggle play/pause
	void toggle_play_pause(bool play) {
		mPlaying = play;

		if (play) {
			mRecording = false;
			if (mActiveActor.has_value()) {
				auto& transformComponent = mActiveActor->get().get_component<TransformComponent>();
				transformComponent.unregister_on_transform_changed_callback(mTransformRegistrationId);
			}
			mAnimatableActors = mActorManager.get_actors_with_component<AnimationComponent>();
		} else {
			if (mActiveActor.has_value()) {
				auto& transformComponent = mActiveActor->get().get_component<TransformComponent>();
				auto& animationComponent = mActiveActor->get().get_component<AnimationComponent>();
				
				mTransformRegistrationId = transformComponent.register_on_transform_changed_callback([this, &animationComponent](const TransformComponent& transform) {
					if (mRecording) {
						animationComponent.addKeyframe(mCurrentTime, transform.get_translation(), transform.get_rotation(), transform.get_scale());
					}
				});
			}
		}
	}
	
	// Helper method to stop playback
	void stop_playback() {
		if (mPlaying) {
			mPlaying = false;
			mPlayPauseBtn->set_pushed(false);
			mPlayPauseBtn->set_icon(FA_PLAY);
			mPlayPauseBtn->set_tooltip("Play");
			// Reset play button color
			mPlayPauseBtn->set_text_color(theme()->m_icon_color);
		}
		
		mAnimatableActors.clear();
	}
	
	// Helper method to update the time display
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
	
	// Helper method to evaluate animations
	void evaluate_animations() {
		for (auto& animatableActor : mAnimatableActors) {
			auto& animationComponent = animatableActor.get().get_component<AnimationComponent>();
			auto& transformComponent = animatableActor.get().get_component<TransformComponent>();
			
			auto evaluationContainer = animationComponent.evaluate(mCurrentTime);
			
			if (evaluationContainer.has_value()) {
				auto [t, r, s] = *evaluationContainer;
				
				transformComponent.set_translation(t);
				transformComponent.set_rotation(r);
				transformComponent.set_scale(s);
			}
		}
	}
	
private:
	ActorManager& mActorManager;
	nanogui::TextBox* mTimeLabel;
	nanogui::Slider* mTimelineSlider;
	nanogui::Button* mRecordBtn;
	nanogui::ToolButton* mPlayPauseBtn;
	nanogui::Color mBackgroundColor;
	
	std::vector<std::reference_wrapper<Actor>> mAnimatableActors;
	std::optional<std::reference_wrapper<Actor>> mActiveActor;
	
	bool mRecording;
	bool mPlaying;
	
	int mTransformRegistrationId;
	int mCurrentTime;
	int mTotalFrames;
};

UiManager::UiManager(IActorSelectedRegistry& registry, IActorVisualManager& actorVisualManager, ActorManager& actorManager, MeshActorLoader& meshActorLoader, ShaderManager& shaderManager, ScenePanel& scenePanel, Canvas& canvas, nanogui::Widget& toolbox, nanogui::Widget& statusBar, CameraManager& cameraManager, std::function<void(std::function<void(int, int)>)> applicationClickRegistrator)
: mRegistry(registry)
, mActorManager(actorManager)
, mShader(*shaderManager.get_shader("mesh"))
, mGizmoManager(std::make_unique<GizmoManager>(toolbox, shaderManager, actorManager))
, mGrid(std::make_unique<Grid>(shaderManager)) {
	mRegistry.RegisterOnActorSelectedCallback(*this);
	
	// Initialize the scene time bar
	mSceneTimeBar = new SceneTimeBar(&scenePanel, mActorManager,  scenePanel.fixed_width(), scenePanel.fixed_height() * 0.2f);
	
	mRegistry.RegisterOnActorSelectedCallback(*mSceneTimeBar);
	
	// Attach it to the top of the canvas, making it stick at the top
	mSceneTimeBar->set_position(nanogui::Vector2i(0, 0));  // Stick to top
	mSceneTimeBar->set_size(nanogui::Vector2i(canvas.width(), 40));  // Full width of the canvas
	
	
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
	mRegistry.UnregisterOnActorSelectedCallback(*mSceneTimeBar);
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

