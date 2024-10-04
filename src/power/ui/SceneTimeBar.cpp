#include "SceneTimeBar.hpp"

// Include necessary headers
#include "actors/ActorManager.hpp"
#include "actors/Actor.hpp"
#include "components/TransformAnimationComponent.hpp"
#include "components/SkinnedAnimationComponent.hpp"
#include "components/TransformComponent.hpp"
#include "components/PlaybackComponent.hpp"

#include <nanogui/button.h>
#include <nanogui/slider.h>
#include <nanogui/toolbutton.h>
#include <nanogui/textbox.h>
#include <iostream>

// Anonymous namespace for helper functions
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
	
	// Step 2 - NDC to view (Anton's version)
	glm::vec4 ray_ndc_4d(ndc_x, ndc_y, 1.0f, 1.0f);
	glm::vec4 ray_view_4d = ProjectionInv * ray_ndc_4d;
	
	// Step 3 - intersect view vector with object Z plane (in view)
	glm::vec4 view_space_intersect = glm::vec4(glm::vec3(ray_view_4d) * depth, 1.0f);
	
	// Step 4 - View to World space
	glm::mat4 View = viewMatrix;
	glm::mat4 InvView = glm::inverse(viewMatrix);
	glm::vec4 point_world = InvView * view_space_intersect;
	return glm::vec3(point_world);
}

} // namespace

// Constructor Implementation
SceneTimeBar::SceneTimeBar(nanogui::Widget* parent, ActorManager& actorManager, AnimationTimeProvider& animationTimeProvider, IActorSelectedRegistry& registry, int width, int height)
: nanogui::Widget(parent),
mActorManager(actorManager),
mAnimationTimeProvider(animationTimeProvider),
mRegistry(registry),
mTransformRegistrationId(-1),
mPlaybackRegistrationId(-1),
mCurrentTime(0),
mTotalFrames(1800), // 30 seconds at 60 FPS
mRecording(false),
mPlaying(false),
mUncommittedKey(false),
mNormalButtonColor(theme()->m_text_color) // Initialize normal button color
{
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
		
		refresh_actors();
		
		mRecordBtn->set_text_color(mNormalButtonColor);
		
		evaluate_transforms();
		evaluate_animations();
		
		// Verify keyframes after time update
		find_previous_and_next_keyframes();
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
		
		find_previous_and_next_keyframes();

		mCurrentTime = 0;
		update_time_display(mCurrentTime);
		mTimelineSlider->set_value(0.0f);
		
		refresh_actors();
		evaluate_transforms();
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
			find_previous_and_next_keyframes();

			mCurrentTime--;
			update_time_display(mCurrentTime);
			mTimelineSlider->set_value(static_cast<float>(mCurrentTime) / mTotalFrames);
			
			refresh_actors();
			evaluate_transforms();
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
		find_previous_and_next_keyframes();

		mCurrentTime = 0;
		update_time_display(mCurrentTime);
		mTimelineSlider->set_value(0.0f);
		evaluate_transforms();
		evaluate_animations();
		
		mAnimatableActors.clear();
	});
	
	// Play/Pause Button
	mPlayPauseBtn = new nanogui::ToolButton(buttonWrapper, FA_PLAY);
	mPlayPauseBtn->set_fixed_width(buttonWidth);
	mPlayPauseBtn->set_fixed_height(buttonHeight);
	mPlayPauseBtn->set_tooltip("Play");
	
	mPlayPauseBtn->set_change_callback([this](bool active) {
		find_previous_and_next_keyframes();

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
		find_previous_and_next_keyframes();

		if (!mPlaying) {
			mRecording = active;
			mRecordBtn->set_text_color(active ? nanogui::Color(1.0f, 0.0f, 0.0f, 1.0f) : normalRecordColor); // Red when recording
			
			
			if (mActiveActor != std::nullopt) {
				auto& transformAnimationComponent = mActiveActor->get().get_component<TransformAnimationComponent>();
				
				if (mRecording) {
					transformAnimationComponent.Freeze();
				} else {
					transformAnimationComponent.Unfreeze();
				}
			}

			register_actor_callbacks();
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
			
			refresh_actors();
			evaluate_transforms();
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
		
		find_previous_and_next_keyframes();
		
		mCurrentTime = mTotalFrames;
		update_time_display(mCurrentTime);
		mTimelineSlider->set_value(1.0f);
		
		refresh_actors();
		evaluate_transforms();
		evaluate_animations();
	});
	
	// Keyframe Buttons Wrapper
	nanogui::Widget* keyBtnWrapper = new nanogui::Widget(this);
	keyBtnWrapper->set_layout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal, nanogui::Alignment::Middle, 1, 1));
	
	mPrevKeyBtn = new nanogui::Button(keyBtnWrapper, "", FA_STEP_BACKWARD);
	mPrevKeyBtn->set_enabled(false);
	mPrevKeyBtn->set_fixed_width(keyBtnWidth);
	mPrevKeyBtn->set_fixed_height(keyBtnHeight);
	mPrevKeyBtn->set_tooltip("Previous Keyframe");
	mPrevKeyBtn->set_callback([this]() {
		stop_playback();
		
		if (!mActiveActor.has_value()) return; // No active actor selected
		
		float currentTimeFloat = static_cast<float>(mCurrentTime);
		float latestPrevTime = -std::numeric_limits<float>::infinity();
		bool hasPrevKeyframe = false;
		
		// Get the AnimationComponent
		auto& component = mActiveActor->get().get_component<TransformAnimationComponent>();
		
		// Get previous keyframe from AnimationComponent
		float previousKeyframeTime = component.GetPreviousKeyframeTime();
		
		if (previousKeyframeTime != -1) {
			hasPrevKeyframe = true;
			if (previousKeyframeTime > latestPrevTime) {
				latestPrevTime = previousKeyframeTime;
			}
		}
		
		// If SkinnedAnimationComponent exists, consider its keyframes as well
		if (mActiveActor->get().find_component<SkinnedAnimationComponent>()) {
			const SkinnedAnimationComponent& skinnedComponent = mActiveActor->get().get_component<SkinnedAnimationComponent>();
			
			// Get previous keyframe from SkinnedAnimationComponent
			std::optional<SkinnedAnimationComponent::Keyframe> prevSkinnedKeyframe = skinnedComponent.get_previous_keyframe(currentTimeFloat);
			
			if (prevSkinnedKeyframe) {
				hasPrevKeyframe = true;
				if (prevSkinnedKeyframe->time > latestPrevTime) {
					latestPrevTime = prevSkinnedKeyframe->time;
				}
			}
		}
		
		if (hasPrevKeyframe) {
			// Update current time to the latest previous keyframe's time
			mCurrentTime = static_cast<int>(latestPrevTime);
			update_time_display(mCurrentTime);
			mTimelineSlider->set_value(static_cast<float>(mCurrentTime) / mTotalFrames);
			
			refresh_actors();
			evaluate_transforms();
			evaluate_animations();
			
			// Verify keyframes after time update
			find_previous_and_next_keyframes();
		} else {
			// No previous keyframe available
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
				auto& component = mActiveActor->get().get_component<TransformAnimationComponent>();
				
				if (wasUncommitted){
					mUncommittedKey = false;
					component.UpdateKeyframe();
				} else if (component.KeyframeExists()) {
					component.RemoveKeyframe();
				} else {
					component.AddKeyframe();
				}
				
				if (mActiveActor->get().find_component<SkinnedAnimationComponent>()){
					auto& skinnedAnimationComponent = mActiveActor->get().get_component<SkinnedAnimationComponent>();
					auto& playback = mActiveActor->get().get_component<PlaybackComponent>();
					
					auto state = playback.get_state();
					
					if (wasUncommitted){
						mUncommittedKey = false;
						skinnedAnimationComponent.updateKeyframe(mCurrentTime, state.getPlaybackState(), state.getPlaybackModifier(), state.getPlaybackTrigger());
					} else if (skinnedAnimationComponent.is_keyframe(mCurrentTime)) {
						skinnedAnimationComponent.removeKeyframe(mCurrentTime);
					} else {
						skinnedAnimationComponent.addKeyframe(mCurrentTime, state.getPlaybackState(), state.getPlaybackModifier(), state.getPlaybackTrigger());
					}
				}
			}
			evaluate_transforms();
			evaluate_animations();
			find_previous_and_next_keyframes();
		}
	});
	
	// Next Frame Button
	mNextKeyBtn = new nanogui::Button(keyBtnWrapper, "", FA_STEP_FORWARD);
	mNextKeyBtn->set_enabled(false);
	mNextKeyBtn->set_fixed_width(keyBtnWidth);
	mNextKeyBtn->set_fixed_height(keyBtnHeight);
	mNextKeyBtn->set_tooltip("Next Keyframe");
	mNextKeyBtn->set_callback([this]() {
		stop_playback();
		
		if (!mActiveActor.has_value()) return; // No active actor selected
		
		float earliestNextTime = std::numeric_limits<float>::infinity();
		bool hasNextKeyframe = false;
		
		// Get the AnimationComponent
		auto& component = mActiveActor->get().get_component<TransformAnimationComponent>();
		
		// Get next keyframe from AnimationComponent
		float nextKeyframeTime = component.GetNextKeyframeTime();
		
		if (nextKeyframeTime != -1) {
			hasNextKeyframe = true;
			if (nextKeyframeTime < earliestNextTime) {
				earliestNextTime = nextKeyframeTime;
			}
		}
		
		// If SkinnedAnimationComponent exists, consider its keyframes as well
		if (mActiveActor->get().find_component<SkinnedAnimationComponent>()) {
			const SkinnedAnimationComponent& skinnedComponent = mActiveActor->get().get_component<SkinnedAnimationComponent>();
			
			// Get next keyframe from SkinnedAnimationComponent
			std::optional<SkinnedAnimationComponent::Keyframe> nextSkinnedKeyframe = skinnedComponent.get_next_keyframe(mCurrentTime);
			
			if (nextSkinnedKeyframe) {
				hasNextKeyframe = true;
				if (nextSkinnedKeyframe->time < earliestNextTime) {
					earliestNextTime = nextSkinnedKeyframe->time;
				}
			}
		}
		
		if (hasNextKeyframe) {
			// Update current time to the earliest next keyframe's time
			mCurrentTime = static_cast<int>(earliestNextTime);
			update_time_display(mCurrentTime);
			mTimelineSlider->set_value(static_cast<float>(mCurrentTime) / mTotalFrames);
			
			refresh_actors();
			evaluate_transforms();
			evaluate_animations();
			
			// Verify keyframes after time update
			find_previous_and_next_keyframes();
		} else {
			// No next keyframe available
			std::cout << "No next keyframe available." << std::endl;
		}
	});
	
	
	mRegistry.RegisterOnActorSelectedCallback(*this);
	
	set_position(nanogui::Vector2i(0, 0));  // Stick to top
}

SceneTimeBar::~SceneTimeBar() {
	mRegistry.UnregisterOnActorSelectedCallback(*this);
}

// Override OnActorSelected from IActorSelectedCallback
void SceneTimeBar::OnActorSelected(std::optional<std::reference_wrapper<Actor>> actor) {
	mActiveActor = actor;
	
	register_actor_callbacks();
	
	find_previous_and_next_keyframes();
}

// Override mouse_button_event to consume the event
bool SceneTimeBar::mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) {
	nanogui::Widget::mouse_button_event(p, button, down, modifiers);
	return true; // Consume the event
}

// Override mouse_motion_event to consume the event
bool SceneTimeBar::mouse_motion_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) {
	nanogui::Widget::mouse_motion_event(p, rel, button, modifiers);
	return true; // Consume the event
}

// Manual force draw to draw on top
void SceneTimeBar::update() {
	mAnimationTimeProvider.Update(mCurrentTime);
	
	if (mPlaying) {
		if (mCurrentTime < mTotalFrames) {
			mCurrentTime++;
			update_time_display(mCurrentTime);
			mTimelineSlider->set_value(static_cast<float>(mCurrentTime) / mTotalFrames);
		} else {
			// Stop playing when end is reached
			stop_playback();
		}
		evaluate_transforms();
	}
	
	evaluate_animations();
	verify_uncommited_key();
	evaluate_keyframe_status();

	
	auto ctx = screen()->nvg_context();
	// Draw background
	nvgBeginPath(ctx);
	nvgRect(ctx, m_pos.x(), m_pos.y(), m_size.x(), m_size.y());
	nvgFillColor(ctx, mBackgroundColor);
	nvgFill(ctx);
	
	// Call the parent class draw method to render the child widgets
	nanogui::Widget::draw(ctx);
}

// Getter for current time
int SceneTimeBar::current_time() const {
	return mCurrentTime;
}

// Helper method to toggle play/pause
void SceneTimeBar::toggle_play_pause(bool play) {
	mPlaying = play;
	if (play) {
		refresh_actors();
	} else {
		register_actor_callbacks();
	}
}

void SceneTimeBar::verify_uncommited_key() {
	if (!mRecording && !mPlaying) {
		if (mActiveActor != std::nullopt) {
			auto& component = mActiveActor->get().get_component<TransformAnimationComponent>();
			
			if (component.KeyframeExists()) {
				
				mUncommittedKey = !component.IsSyncWithProvider();
				
			}
		}
	}
}

// Helper method to stop playback
void SceneTimeBar::stop_playback() {
	mAnimationTimeProvider.Update(mCurrentTime);

	mUncommittedKey = false;
	
	if (mPlaying) {
		mPlaying = false;
		mPlayPauseBtn->set_pushed(false);
		mPlayPauseBtn->set_icon(FA_PLAY);
		mPlayPauseBtn->set_tooltip("Play");
		// Reset play button color
		mPlayPauseBtn->set_text_color(mNormalButtonColor);
	}
	
	if (mActiveActor.has_value()) {
		auto& component = mActiveActor->get().get_component<TransformAnimationComponent>();
		
		component.Unfreeze();
	}
}

// Helper method to update the time display
void SceneTimeBar::update_time_display(int frameCount) {
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

// Helper method to evaluate transforms
void SceneTimeBar::evaluate_transforms() {
	for (auto& animatableActor : mAnimatableActors) {
		auto& component = animatableActor.get().get_component<TransformAnimationComponent>();
		
		if (!mRecording) {
			component.Unfreeze();
		}
		component.Evaluate();
	}
}

// Helper method to evaluate animations
void SceneTimeBar::evaluate_animations() {
	for (auto& animatableActor : mAnimatableActors) {
		if (animatableActor.get().find_component<SkinnedAnimationComponent>()) {
			auto& skinnedAnimationComponent = animatableActor.get().get_component<SkinnedAnimationComponent>();
			
			skinnedAnimationComponent.evaluate(mCurrentTime);
		}
	}
}

// Helper method to evaluate keyframe status
void SceneTimeBar::evaluate_keyframe_status() {
	bool atKeyframe = false;
	bool betweenKeyframes = false;
	
	// Loop through animatable actors to determine keyframe status
	if (mActiveActor.has_value()) {
		auto& component = mActiveActor->get().get_component<TransformAnimationComponent>();
		
		if (component.KeyframeExists()) {
			atKeyframe = true;
		} else if (component.GetPreviousKeyframeTime() != -1 && component.GetNextKeyframeTime() != -1) {
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

// Helper method to verify previous and next keyframes
std::tuple<bool, bool> SceneTimeBar::find_previous_and_next_keyframes() {
	if (!mActiveActor.has_value()) {
		// No active actor; disable both buttons
		mPrevKeyBtn->set_enabled(false);
		mNextKeyBtn->set_enabled(false);
		return std::make_tuple(false, false);
	}
	
	float currentTimeFloat = static_cast<float>(mCurrentTime);
	
	// Get the AnimationComponent
	auto& component = mActiveActor->get().get_component<TransformAnimationComponent>();
	
	float previousKeyframeTime = component.GetPreviousKeyframeTime();
	
	float nextKeyframeTime = component.GetNextKeyframeTime();

	float latestPrevTime = -std::numeric_limits<float>::infinity();
	float earliestNextTime = std::numeric_limits<float>::infinity();
	
	// Flags to indicate if we have previous or next keyframes
	bool hasPrevKeyframe = false;
	bool hasNextKeyframe = false;
	
	// Check previous keyframe from AnimationComponent
	if (previousKeyframeTime != -1) {
		hasPrevKeyframe = true;
		if (previousKeyframeTime > latestPrevTime) {
			latestPrevTime = previousKeyframeTime;
		}
	}
	
	// Check next keyframe from AnimationComponent
	if (nextKeyframeTime != -1) {
		hasNextKeyframe = true;
		if (nextKeyframeTime < earliestNextTime) {
			earliestNextTime = nextKeyframeTime;
		}
	}
	
	// If SkinnedAnimationComponent exists, consider its keyframes as well
	if (mActiveActor->get().find_component<SkinnedAnimationComponent>()) {
		const SkinnedAnimationComponent& skinnedComponent = mActiveActor->get().get_component<SkinnedAnimationComponent>();
		
		// Get previous and next keyframes from SkinnedAnimationComponent
		std::optional<SkinnedAnimationComponent::Keyframe> prevSkinnedKeyframe = skinnedComponent.get_previous_keyframe(currentTimeFloat);
		std::optional<SkinnedAnimationComponent::Keyframe> nextSkinnedKeyframe = skinnedComponent.get_next_keyframe(currentTimeFloat);
		
		// Check previous keyframe from SkinnedAnimationComponent
		if (prevSkinnedKeyframe) {
			hasPrevKeyframe = true;
			if (prevSkinnedKeyframe->time > latestPrevTime) {
				latestPrevTime = prevSkinnedKeyframe->time;
			}
		}
		
		// Check next keyframe from SkinnedAnimationComponent
		if (nextSkinnedKeyframe) {
			hasNextKeyframe = true;
			if (nextSkinnedKeyframe->time < earliestNextTime) {
				earliestNextTime = nextSkinnedKeyframe->time;
			}
		}
	}
	
	// Enable or disable the Previous Keyframe button
	mPrevKeyBtn->set_enabled(hasPrevKeyframe);
	
	// Enable or disable the Next Keyframe button
	mNextKeyBtn->set_enabled(hasNextKeyframe);
	
	return std::make_tuple(hasPrevKeyframe, hasNextKeyframe);
}

// Helper method to refresh animatable actors
void SceneTimeBar::refresh_actors() {
	mAnimatableActors = mActorManager.get_actors_with_component<TransformAnimationComponent>();
}

// Helper method to register actor callbacks
void SceneTimeBar::register_actor_callbacks() {
	if(mPlaybackRegistrationId != -1) {
		mRegisteredPlaybackComponent->get()
			.unregister_on_playback_changed_callback(mPlaybackRegistrationId);
		
		mPlaybackRegistrationId = -1;
		
		mRegisteredPlaybackComponent = std::nullopt;
	}
	
	if (mActiveActor != std::nullopt) {
		auto& transformAnimationComponent = mActiveActor->get().get_component<TransformAnimationComponent>();

		transformAnimationComponent.TriggerRegistration();
		
		if (mActiveActor->get().find_component<PlaybackComponent>()) {
			mRegisteredPlaybackComponent = mActiveActor->get().get_component<PlaybackComponent>();
		}
		
		if (mRegisteredPlaybackComponent.has_value()) {
			auto& skinnedAnimationComponent = mActiveActor->get().get_component<SkinnedAnimationComponent>();
			
			mPlaybackRegistrationId = mRegisteredPlaybackComponent->get().register_on_playback_changed_callback([this, &skinnedAnimationComponent](const PlaybackComponent& playback) {
				if (mRecording && !mPlaying) {
					skinnedAnimationComponent.addKeyframe(mCurrentTime, playback.getPlaybackState(), playback.getPlaybackModifier(), playback.getPlaybackTrigger());
				} else if (!mRecording && !mPlaying) {
					if (skinnedAnimationComponent.is_keyframe(mCurrentTime)) {
						auto m1 = const_cast<SkinnedAnimationComponent::Keyframe&>(playback.get_state());
						m1.time = mCurrentTime;
						
						auto m2 = skinnedAnimationComponent.get_keyframe(mCurrentTime);
						
						if (m1 != m2) {
							mUncommittedKey = true;
						}
					}
				}
			});
		}
	}
}
