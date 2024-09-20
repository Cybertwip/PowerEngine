#include "UiManager.hpp"

#include "Canvas.hpp"
#include "CameraManager.hpp"

#include "MeshActorLoader.hpp"

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

#include "graphics/drawing/BatchUnit.hpp"
#include "graphics/drawing/MeshBatch.hpp"
#include "graphics/drawing/SkinnedMeshBatch.hpp"
#include "graphics/drawing/Grid.hpp"

#include "ui/ScenePanel.hpp"
#include "ui/StatusBarPanel.hpp"

#if defined(NANOGUI_USE_OPENGL) || defined(NANOGUI_USE_GLES)

#include <nanogui/opengl.h>

#elif defined(NANOGUI_USE_METAL)

#include "MetalHelper.hpp"

#endif

#include <nanovg.h>


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
		int buttonHeight = static_cast<int>(fixedHeight * 0.15f);
		
		// Define smaller size for mKeyBtn
		int keyBtnWidth = static_cast<int>(buttonWidth * 0.5f);
		int keyBtnHeight = static_cast<int>(buttonHeight * 1.0f);
		
		// Vertical layout for slider above buttons
		set_layout(new nanogui::BoxLayout(nanogui::Orientation::Vertical, nanogui::Alignment::Maximum, 1, 1));
		
		// Slider Wrapper
		nanogui::Widget* sliderWrapper = new nanogui::Widget(this);
		sliderWrapper->set_layout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal, nanogui::Alignment::Fill, 1, 1));
		
		auto normalButtonColor = theme()->m_text_color;
		
		// Timeline Slider
		mTimelineSlider = new nanogui::Slider(sliderWrapper);
		mTimelineSlider->set_value(0.0f);  // Start at 0%
		mTimelineSlider->set_fixed_width(fixedWidth * 0.985f);
		mTimelineSlider->set_range(std::make_pair(0.0f, 1.0f));  // Normalized range
		
		// Slider Callback to update mCurrentTime
		mTimelineSlider->set_callback([this](float value) {
			mCurrentTime = static_cast<int>(value * mTotalFrames);
			update_time_display(mCurrentTime);
			
			stop_playback();
			
			mAnimatableActors = mActorManager.get_actors_with_component<AnimationComponent>();
			
			mRecordBtn->set_text_color(mNormalButtonColor);
			
			evaluate_animations();
			
			// Verify keyframes after time update
			verify_previous_next_keyframes(mActiveActor);
			
		});
		
		// Buttons Wrapper
		nanogui::Widget* buttonWrapperWrapper = new nanogui::Widget(this);
		buttonWrapperWrapper->set_layout(new nanogui::BoxLayout(nanogui::Orientation::Vertical, nanogui::Alignment::Middle, 1, 1));
		
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
			
			verify_previous_next_keyframes(mActiveActor);
			
			mCurrentTime = 0;
			update_time_display(mCurrentTime);
			mTimelineSlider->set_value(0.0f);
			
			mAnimatableActors = mActorManager.get_actors_with_component<AnimationComponent>();
			
			evaluate_animations();
		});
		
		// Previous Frame Button
		nanogui::Button* prevFrameBtn = new nanogui::Button(buttonWrapper, "", FA_STEP_BACKWARD);
		prevFrameBtn->set_fixed_width(buttonWidth);
		prevFrameBtn->set_fixed_height(buttonHeight);
		prevFrameBtn->set_tooltip("Previous Frame");
		prevFrameBtn->set_callback([this]() {
			if (mCurrentTime > 0) {
				stop_playback();
				verify_previous_next_keyframes(mActiveActor);
				
				mCurrentTime--;
				update_time_display(mCurrentTime);
				mTimelineSlider->set_value(static_cast<float>(mCurrentTime) / mTotalFrames);
				
				mAnimatableActors = mActorManager.get_actors_with_component<AnimationComponent>();
				
				evaluate_animations();
			}
		});
		
		// Stop Button
		nanogui::Button* stopBtn = new nanogui::Button(buttonWrapper, "", FA_STOP);
		stopBtn->set_fixed_width(buttonWidth);
		stopBtn->set_fixed_height(buttonHeight);
		stopBtn->set_tooltip("Stop");
		stopBtn->set_callback([this]() {
			stop_playback();
			verify_previous_next_keyframes(mActiveActor);
			
			mCurrentTime = 0;
			update_time_display(mCurrentTime);
			mTimelineSlider->set_value(0.0f);
			evaluate_animations();
			
			mAnimatableActors.clear();
		});
		// Play/Pause Button
		mPlayPauseBtn = new nanogui::ToolButton(buttonWrapper, FA_PLAY);
		mPlayPauseBtn->set_fixed_width(buttonWidth);
		mPlayPauseBtn->set_fixed_height(buttonHeight);
		mPlayPauseBtn->set_tooltip("Play");
		
		mPlayPauseBtn->set_change_callback([this](bool active) {
			
			verify_previous_next_keyframes(mActiveActor);
			
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
				mPlayPauseBtn->set_text_color(mNormalButtonColor);
			}
		});
		
		
		// Record Button
		mRecordBtn = new nanogui::ToolButton(buttonWrapper, FA_CIRCLE);
		mRecordBtn->set_fixed_width(buttonWidth);
		mRecordBtn->set_fixed_height(buttonHeight);
		mRecordBtn->set_tooltip("Record");

		auto normalRecordColor = mRecordBtn->text_color();
		
		mRecordBtn->set_change_callback([this, normalRecordColor](bool active) {
			
			verify_previous_next_keyframes(mActiveActor);
			
			if (!mPlaying) {
				mRecording = active;
				mRecordBtn->set_text_color(active ? nanogui::Color(1.0f, 0.0f, 0.0f, 1.0f) : normalRecordColor); // Red when recording
				
				register_actor_transform_callback(mActiveActor);
			}
		});
		
		// Next Frame Button
		nanogui::Button* nextFrameBtn = new nanogui::Button(buttonWrapper, "", FA_STEP_FORWARD);
		nextFrameBtn->set_fixed_width(buttonWidth);
		nextFrameBtn->set_fixed_height(buttonHeight);
		nextFrameBtn->set_tooltip("Next Frame");
		nextFrameBtn->set_callback([this]() {
			if (mCurrentTime < mTotalFrames) {
				stop_playback();
				
				mCurrentTime++;
				update_time_display(mCurrentTime);
				mTimelineSlider->set_value(static_cast<float>(mCurrentTime) / mTotalFrames);
				
				mAnimatableActors = mActorManager.get_actors_with_component<AnimationComponent>();
				
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
			
			verify_previous_next_keyframes(mActiveActor);
			
			mCurrentTime = mTotalFrames;
			update_time_display(mCurrentTime);
			mTimelineSlider->set_value(1.0f);
			
			mAnimatableActors = mActorManager.get_actors_with_component<AnimationComponent>();
			
			evaluate_animations();
		});
		
		nanogui::Widget* keyBtnWrapper = new nanogui::Widget(this);
		keyBtnWrapper->set_layout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal, nanogui::Alignment::Middle, 1, 1));
		
		mPrevKeyBtn = new nanogui::Button(keyBtnWrapper, "", FA_STEP_BACKWARD);
		
		mPrevKeyBtn->set_enabled(false);
		mPrevKeyBtn->set_fixed_width(keyBtnWidth);
		mPrevKeyBtn->set_fixed_height(keyBtnHeight);
		mPrevKeyBtn->set_tooltip("Previous Frame");
		mPrevKeyBtn->set_callback([this]() {
			stop_playback();
			
			if (!mActiveActor.has_value()) return; // No active actor selected
			
			const AnimationComponent& animComp = mActiveActor->get().get_component<AnimationComponent>();
			float currentTimeFloat = static_cast<float>(mCurrentTime);
			
			// Get the previous keyframe
			std::optional<AnimationComponent::Keyframe> prevKeyframe = animComp.get_previous_keyframe(currentTimeFloat);
			
			if (prevKeyframe.has_value()) {
				// Update current time to the previous keyframe's time
				mCurrentTime = static_cast<int>(prevKeyframe->time);
				update_time_display(mCurrentTime);
				mTimelineSlider->set_value(static_cast<float>(mCurrentTime) / mTotalFrames);
				
				mAnimatableActors = mActorManager.get_actors_with_component<AnimationComponent>();
				
				evaluate_animations();
				
				// Verify keyframes after time update
				verify_previous_next_keyframes(mActiveActor);
			}
			// Optionally, you can provide feedback if there is no previous keyframe
			else {
				// Example: Display a message or disable the button
				// For simplicity, we'll just print to the console
				std::cout << "No previous keyframe available." << std::endl;
			}
			
		});
		
		mKeyBtn = new nanogui::Button(keyBtnWrapper, "", FA_KEY);
		mKeyBtn->set_fixed_width(keyBtnWidth);
		mKeyBtn->set_fixed_height(keyBtnHeight);
		mKeyBtn->set_tooltip("Keyframe Tool");
		
		// Initially set to the normal button color
		mKeyBtn->set_text_color(mNormalButtonColor);
		
		mKeyBtn->set_callback([this](){
			bool wasUncommitted = mUncommittedKey;
			
			stop_playback();

			if (mActiveActor.has_value()) {
				if (!mPlaying) {
					auto& transformComponent = mActiveActor->get().get_component<TransformComponent>();
					
					auto& animationComponent = mActiveActor->get().get_component<AnimationComponent>();
					
					if (wasUncommitted){
						mUncommittedKey = false;
						animationComponent.updateKeyframe(mCurrentTime, transformComponent.get_translation(), transformComponent.get_rotation(), transformComponent.get_scale());
						
					} else if (animationComponent.is_keyframe(mCurrentTime)) {
						animationComponent.removeKeyframe(mCurrentTime);
					} else {
						animationComponent.addKeyframe(mCurrentTime, transformComponent.get_translation(), transformComponent.get_rotation(), transformComponent.get_scale());
					}
					
				}
				
				verify_previous_next_keyframes(mActiveActor);
			}
		});
		
		// Next Frame Button
		mNextKeyBtn = new nanogui::Button(keyBtnWrapper, "", FA_STEP_FORWARD);
		
		mNextKeyBtn->set_enabled(false);
		
		mNextKeyBtn->set_fixed_width(keyBtnWidth);
		mNextKeyBtn->set_fixed_height(keyBtnHeight);
		mNextKeyBtn->set_tooltip("Next Frame");
		mNextKeyBtn->set_callback([this]() {
			stop_playback();
			
			if (!mActiveActor.has_value()) return; // No active actor selected
			
			const AnimationComponent& animComp = mActiveActor->get().get_component<AnimationComponent>();
			float currentTimeFloat = static_cast<float>(mCurrentTime);
			
			// Get the next keyframe
			std::optional<AnimationComponent::Keyframe> nextKeyframe = animComp.get_next_keyframe(currentTimeFloat);
			
			if (nextKeyframe.has_value()) {
				// Update current time to the next keyframe's time
				mCurrentTime = static_cast<int>(nextKeyframe->time);
				update_time_display(mCurrentTime);
				mTimelineSlider->set_value(static_cast<float>(mCurrentTime) / mTotalFrames);
				
				mAnimatableActors = mActorManager.get_actors_with_component<AnimationComponent>();
				
				evaluate_animations();
				
				// Verify keyframes after time update
				verify_previous_next_keyframes(mActiveActor);
			}
			// Optionally, you can provide feedback if there is no next keyframe
			else {
				// Example: Display a message or disable the button
				// For simplicity, we'll just print to the console
				std::cout << "No next keyframe available." << std::endl;
			}
			
		});
	}
	
	void evaluate_keyframe_status() {
		bool atKeyframe = false;
		bool betweenKeyframes = false;
		
		// Loop through animatable actors to determine keyframe status
		if (mActiveActor.has_value()) {
			auto& animatableActor = mActiveActor;
			
			auto& animationComponent = animatableActor->get().get_component<AnimationComponent>();
			
			if (animationComponent.is_keyframe(mCurrentTime)) {
				atKeyframe = true;
			} else if (animationComponent.is_between_keyframes(mCurrentTime)) {
				betweenKeyframes = true;
			}
		}
		
		// Set the color based on keyframe status
		if (atKeyframe) {
			mKeyBtn->set_text_color(nanogui::Color(0.0f, 0.0f, 1.0f, 1.0f)); // Blue
		} else if (betweenKeyframes) {
			mKeyBtn->set_text_color(nanogui::Color(1.0f, 1.0f, 0.0f, 1.0f)); // Yellow
		} else {
			mKeyBtn->set_text_color(mNormalButtonColor); // Normal
		}
		
		if (atKeyframe || betweenKeyframes) {
			if (mUncommittedKey) {
				mKeyBtn->set_text_color(nanogui::Color(1.0f, 0.0f, 0.0f, 1.0f));
			}
		}
	}
	
	// Override OnActorSelected from IActorSelectedCallback
	void OnActorSelected(std::optional<std::reference_wrapper<Actor>> actor) override {
		
		mActiveActor = actor;
		
		register_actor_transform_callback(mActiveActor);
		
		verify_previous_next_keyframes(mActiveActor);
	}

	void register_actor_transform_callback(std::optional<std::reference_wrapper<Actor>> actor) {
		
		if(mTransformRegistrationId != -1) {
			mRegisteredTransformComponent->get()
			.unregister_on_transform_changed_callback(mTransformRegistrationId);
		}
		
		if (actor != std::nullopt) {
			mRegisteredTransformComponent = actor->get().get_component<TransformComponent>();
			

			auto& animationComponent = actor->get().get_component<AnimationComponent>();
			
			mTransformRegistrationId = mRegisteredTransformComponent->get().register_on_transform_changed_callback([this, &animationComponent](const TransformComponent& transform) {
				if (mRecording && !mPlaying) {
					animationComponent.addKeyframe(mCurrentTime, transform.get_translation(), transform.get_rotation(), transform.get_scale());
				} else if (!mRecording && !mPlaying) {
					if (animationComponent.is_keyframe(mCurrentTime)) {
						
						auto m1 = transform.get_matrix();
						
						auto m2 = animationComponent.evaluate_as_matrix(mCurrentTime);
						
						if (m1 != *m2) {
							mUncommittedKey = true;
						}
					}
				}
			});
		}
		
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
			
			evaluate_animations();
		}
		
		evaluate_keyframe_status();
		
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
			mAnimatableActors = mActorManager.get_actors_with_component<AnimationComponent>();
		} else {
			register_actor_transform_callback(mActiveActor);
		}
	}
	
	// Helper method to stop playback
	void stop_playback() {
		mUncommittedKey = false;
		
		if (mPlaying) {
			mPlaying = false;
			mPlayPauseBtn->set_pushed(false);
			mPlayPauseBtn->set_icon(FA_PLAY);
			mPlayPauseBtn->set_tooltip("Play");
			// Reset play button color
			mPlayPauseBtn->set_text_color(mNormalButtonColor);
		}
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
	// Add the verify_previous_next_keyframes method
	void verify_previous_next_keyframes(const std::optional<std::reference_wrapper<Actor>>& activeActor) {
		if (!activeActor.has_value()) {
			// No active actor; disable both buttons
			mPrevKeyBtn->set_enabled(false);
			mNextKeyBtn->set_enabled(false);
			return;
		}
		
		const AnimationComponent& animComp = activeActor->get().get_component<AnimationComponent>();
		float currentTimeFloat = static_cast<float>(mCurrentTime);
		
		// Get previous and next keyframes
		std::optional<AnimationComponent::Keyframe> prevKeyframe = animComp.get_previous_keyframe(currentTimeFloat);
		std::optional<AnimationComponent::Keyframe> nextKeyframe = animComp.get_next_keyframe(currentTimeFloat);
		
		// Enable or disable the Previous Keyframe button
		if (prevKeyframe.has_value()) {
			mPrevKeyBtn->set_enabled(true);
		} else {
			mPrevKeyBtn->set_enabled(false);
		}
		
		// Enable or disable the Next Keyframe button
		if (nextKeyframe.has_value()) {
			mNextKeyBtn->set_enabled(true);
		} else {
			mNextKeyBtn->set_enabled(false);
		}
	}
	
private:
	ActorManager& mActorManager;
	nanogui::TextBox* mTimeLabel;
	nanogui::Slider* mTimelineSlider;
	nanogui::Button* mRecordBtn;
	nanogui::ToolButton* mPlayPauseBtn;
	nanogui::Button* mKeyBtn;
	nanogui::Button* mPrevKeyBtn;
	nanogui::Button* mNextKeyBtn;
	nanogui::Color mBackgroundColor;
	nanogui::Color mNormalButtonColor;
	
	std::vector<std::reference_wrapper<Actor>> mAnimatableActors;
	std::optional<std::reference_wrapper<Actor>> mActiveActor;
	
	bool mRecording;
	bool mPlaying;
	
	bool mUncommittedKey;
	
	std::optional<std::reference_wrapper<TransformComponent>> mRegisteredTransformComponent;
	int mTransformRegistrationId;
	int mCurrentTime;
	int mTotalFrames;
};

UiManager::UiManager(IActorSelectedRegistry& registry, IActorVisualManager& actorVisualManager, ActorManager& actorManager, MeshActorLoader& meshActorLoader, ShaderManager& shaderManager, ScenePanel& scenePanel, Canvas& canvas, nanogui::Widget& toolbox, nanogui::Widget& statusBar, CameraManager& cameraManager, std::function<void(std::function<void(int, int)>)> applicationClickRegistrator)
: mRegistry(registry)
, mActorManager(actorManager)
, mShaderManager(shaderManager)
, mGrid(std::make_unique<Grid>(shaderManager))
, mMeshActorLoader(meshActorLoader)
, mGizmoManager(std::make_unique<GizmoManager>(toolbox, shaderManager, actorManager, mMeshActorLoader))

, mCanvas(canvas) {
	//
	//	mRenderPass = new nanogui::RenderPass({mCanvas.render_pass()->targets()[2],
	//		mCanvas.render_pass()->targets()[3]}, mCanvas.render_pass()->targets()[0], mCanvas.render_pass()->targets()[1], nullptr);
	
	mRegistry.RegisterOnActorSelectedCallback(*this);
	
	// Initialize the scene time bar
	mSceneTimeBar = new SceneTimeBar(&scenePanel, mActorManager,  scenePanel.fixed_width(), scenePanel.fixed_height() * 0.25f);
	
	mRegistry.RegisterOnActorSelectedCallback(*mSceneTimeBar);
	
	// Attach it to the top of the canvas, making it stick at the top
	mSceneTimeBar->set_position(nanogui::Vector2i(0, 0));  // Stick to top
	
	
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
				mGizmoManager->hover(GizmoManager::GizmoAxis(id));
			} else if (id == 0 && !down){
				mGizmoManager->hover(GizmoManager::GizmoAxis(0));
			} else if (id != 0 && down) {
				mGizmoManager->hover(GizmoManager::GizmoAxis(id));
			}
			
			// Step 3: Apply the world-space delta transformation
			mGizmoManager->transform(world.x - offset.x, world.y - offset.y);
		}
	});
	
	StatusBarPanel* statusBarPanel = new StatusBarPanel(statusBar, actorVisualManager, meshActorLoader, shaderManager, applicationClickRegistrator);
	statusBarPanel->set_fixed_width(statusBar.fixed_height());
	statusBarPanel->set_fixed_height(statusBar.fixed_height());
	statusBarPanel->set_layout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal,
													  nanogui::Alignment::Minimum, 4, 2));
	
	mSelectionColor = glm::vec4(0.83f, 0.68f, 0.21f, 1.0f); // A gold-ish color
	
	mSelectionColor = glm::normalize(mSelectionColor);
	
}

UiManager::~UiManager() {
	mRegistry.UnregisterOnActorSelectedCallback(*this);
	mRegistry.UnregisterOnActorSelectedCallback(*mSceneTimeBar);
}

void UiManager::OnActorSelected(std::optional<std::reference_wrapper<Actor>> actor) {
	mActiveActor = actor;
	
	mGizmoManager->select(std::nullopt);
}

void UiManager::draw() {
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
	
	// Draw gizmos
	mGizmoManager->draw();
	
	mCanvas.render_pass()->push_depth_test_state(nanogui::RenderPass::DepthTest::Less, true, mShaderManager.identifier("mesh"));
	
	auto& batch_unit = mMeshActorLoader.get_batch_unit();

	mActorManager.visit(batch_unit.mMeshBatch);
	
	mCanvas.render_pass()->set_depth_test(nanogui::RenderPass::DepthTest::Less, true);

	mActorManager.visit(*this);
}

void UiManager::draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view,
							 const nanogui::Matrix4f& projection) {
	// Draw the grid first
	mGrid->draw_content(model, view, projection);
}

