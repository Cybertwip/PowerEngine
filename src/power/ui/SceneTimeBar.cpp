#include "SceneTimeBar.hpp"

// Include necessary headers
#include "actors/ActorManager.hpp"
#include "actors/Actor.hpp"
#include "actors/IActorSelectedRegistry.hpp"
#include "components/TimelineComponent.hpp"

#include <nanogui/button.h>
#include <nanogui/slider.h>
#include <nanogui/toolbutton.h>
#include <nanogui/textbox.h>
#include <nanogui/treeviewitem.h>
#include <nanovg.h>
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
SceneTimeBar::SceneTimeBar(nanogui::Widget& parent, ActorManager& actorManager, AnimationTimeProvider& animationTimeProvider, std::shared_ptr<IActorSelectedRegistry> registry, int width, int height)
: nanogui::Widget(std::make_optional<std::reference_wrapper<Widget>>(parent)),
mActorManager(actorManager),
mAnimationTimeProvider(animationTimeProvider),
mRegistry(registry),
mCurrentTime(0),
mTotalFrames(1800), // 30 seconds at 60 FPS
mRecording(false),
mPlaying(false),
mUncommittedKey(false),
mNormalButtonColor(theme().m_text_color) // Initialize normal button color
{
	// Set fixed dimensions
	set_fixed_width(width);
	set_fixed_height(height);
	
	mBackgroundColor = theme().m_button_gradient_bot_unfocused;
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
	set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Maximum, 1, 1));
	
	// Slider Wrapper
	mSliderWrapper = std::make_shared<nanogui::Widget>(std::make_optional<std::reference_wrapper<nanogui::Widget>>(*this));
	mSliderWrapper->set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Horizontal, nanogui::Alignment::Fill, 1, 1));
	
	auto normalButtonColor = theme().m_text_color;
	
	// Timeline Slider
	mTimelineSlider = std::make_shared<nanogui::Slider>(*mSliderWrapper);
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
		
		// Verify keyframes after time update
		find_previous_and_next_keyframes();

		evaluate_timelines();
		
		commit();
	});
	
	// Buttons Wrapper
	mButtonWrapperWrapper = std::make_shared<nanogui::Widget>(std::make_optional<std::reference_wrapper<nanogui::Widget>>(*this));
	mButtonWrapperWrapper->set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Middle, 1, 1));
	
	// Time Counter Display
	mTimeLabel = std::make_shared<nanogui::TextBox>(*mButtonWrapperWrapper);
	mTimeLabel->set_fixed_width(fixedWidth);
	mTimeLabel->set_font_size(36);
	mTimeLabel->set_alignment(nanogui::TextBox::Alignment::Center);
	mTimeLabel->set_background_color(nanogui::Color(0, 0, 0, 0));
	mTimeLabel->set_value("00:00:00:00");
	
	// Buttons Horizontal Layout
	mButtonWrapper = std::make_shared<nanogui::Widget>(std::make_optional<std::reference_wrapper<nanogui::Widget>>(*mButtonWrapperWrapper));
	mButtonWrapper->set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Horizontal, nanogui::Alignment::Middle, 25, 5));
	
	
	mTakeTreeView = std::make_shared<nanogui::TreeView>(*mButtonWrapper);
	mTakeTreeView->set_layout(
						  std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Fill));
	
	
	
	// Rewind Button
	mRewindBtn = std::make_shared<nanogui::Button>(*mButtonWrapper, "", FA_FAST_BACKWARD);
	mRewindBtn->set_fixed_width(buttonWidth);
	mRewindBtn->set_fixed_height(buttonHeight);
	mRewindBtn->set_tooltip("Rewind to Start");
	mRewindBtn->set_callback([this]() {
		stop_playback(); // Ensure playback is stopped
		
		mCurrentTime = 0;
		update_time_display(mCurrentTime);
		mTimelineSlider->set_value(0.0f);
		
		refresh_actors();
		
		find_previous_and_next_keyframes();

		evaluate_timelines();
		
		commit();
	});
	
	// Previous Frame Button
	mPrevFrameBtn = std::make_shared<nanogui::Button>(*mButtonWrapper, "", FA_STEP_BACKWARD);
	mPrevFrameBtn->set_fixed_width(buttonWidth);
	mPrevFrameBtn->set_fixed_height(buttonHeight);
	mPrevFrameBtn->set_tooltip("Previous Frame");
	mPrevFrameBtn->set_callback([this]() {
		if (mCurrentTime > 0) {
			stop_playback();

			mCurrentTime--;
			update_time_display(mCurrentTime);
			mTimelineSlider->set_value(static_cast<float>(mCurrentTime) / mTotalFrames);
			
			refresh_actors();
			
			find_previous_and_next_keyframes();

			evaluate_timelines();
			
			commit();

		}
	});
	
	// Stop Button
	mStopBtn = std::make_shared<nanogui::Button>(*mButtonWrapper, "", FA_STOP);
	mStopBtn->set_fixed_width(buttonWidth);
	mStopBtn->set_fixed_height(buttonHeight);
	mStopBtn->set_tooltip("Stop");
	mStopBtn->set_callback([this]() {
		stop_playback();

		mCurrentTime = 0;
		update_time_display(mCurrentTime);
		mTimelineSlider->set_value(0.0f);
		
		refresh_actors();

		find_previous_and_next_keyframes();

		evaluate_timelines();
		
		commit();

	});
	
	// Play/Pause Button
	mPlayPauseBtn = std::make_shared<nanogui::ToolButton>(*mButtonWrapper, FA_PLAY);
	mPlayPauseBtn->set_fixed_width(buttonWidth);
	mPlayPauseBtn->set_fixed_height(buttonHeight);
	mPlayPauseBtn->set_tooltip("Play");
	
	mPlayPauseBtn->set_change_callback([this](bool active) {
		stop_playback();
		
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
		
		find_previous_and_next_keyframes();

	});
	
	// Record Button
	mRecordBtn = std::make_shared<nanogui::ToolButton>(*mButtonWrapper, FA_CIRCLE);
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
				auto& component = mActiveActor->get().get_component<TimelineComponent>();
				
				if (mRecording) {
					component.Freeze();
				} else {
					component.Unfreeze();
				}
			}

			register_actor_callbacks();
		}
	});
	
	// Next Frame Button
	mNextFrameBtn = std::make_shared<nanogui::Button>(*mButtonWrapper, "", FA_STEP_FORWARD);
	mNextFrameBtn->set_fixed_width(buttonWidth);
	mNextFrameBtn->set_fixed_height(buttonHeight);
	mNextFrameBtn->set_tooltip("Next Frame");
	mNextFrameBtn->set_callback([this]() {
		if (mCurrentTime < mTotalFrames) {
			stop_playback();
			
			mCurrentTime++;
			update_time_display(mCurrentTime);
			mTimelineSlider->set_value(static_cast<float>(mCurrentTime) / mTotalFrames);
			
			refresh_actors();
			
			find_previous_and_next_keyframes();
			
			evaluate_timelines();
			
			commit();

		}
	});
	
	// Seek to End Button
	mSeekEndBtn = std::make_shared<nanogui::Button>(*mButtonWrapper, "", FA_FAST_FORWARD);
	mSeekEndBtn->set_fixed_width(buttonWidth);
	mSeekEndBtn->set_fixed_height(buttonHeight);
	mSeekEndBtn->set_tooltip("Seek to End");
	mSeekEndBtn->set_callback([this]() {
		stop_playback(); // Ensure playback is stopped
		
		mCurrentTime = mTotalFrames;
		update_time_display(mCurrentTime);
		mTimelineSlider->set_value(1.0f);
		
		refresh_actors();
		
		find_previous_and_next_keyframes();

		evaluate_timelines();
		
		commit();

	});
	
	// Keyframe Buttons Wrapper
	mKeyBtnWrapper = std::make_shared<nanogui::Widget>(std::make_optional<std::reference_wrapper<nanogui::Widget>>(*mButtonWrapperWrapper));
	mKeyBtnWrapper->set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Horizontal, nanogui::Alignment::Middle, 1, 1));
	
	mPrevKeyBtn = std::make_shared<nanogui::Button>(*mKeyBtnWrapper, "", FA_STEP_BACKWARD);
	mPrevKeyBtn->set_enabled(false);
	mPrevKeyBtn->set_fixed_width(keyBtnWidth);
	mPrevKeyBtn->set_fixed_height(keyBtnHeight);
	mPrevKeyBtn->set_tooltip("Previous Keyframe");
	mPrevKeyBtn->set_callback([this]() {
		stop_playback();
		
		if (!mActiveActor.has_value()) return; // No active actor selected
		auto [previousKeyframe, _] = find_previous_and_next_keyframes();
		
		if (previousKeyframe.mActive) {
			// Update current time to the latest previous keyframe's time
			mCurrentTime = static_cast<int>(previousKeyframe.mTime);
			update_time_display(mCurrentTime);
			mTimelineSlider->set_value(static_cast<float>(mCurrentTime) / mTotalFrames);
			
			refresh_actors();
			
			// Re-evaluate
			find_previous_and_next_keyframes();

			evaluate_timelines();
			
			commit();

			
		} else {
			// No previous keyframe available
			std::cout << "No previous keyframe available." << std::endl;
		}
	});
	
	mKeyBtn = std::make_shared<nanogui::Button>(*mKeyBtnWrapper, "", FA_KEY);
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
				auto& component = mActiveActor->get().get_component<TimelineComponent>();
				
				if (wasUncommitted){
					mUncommittedKey = false;
					component.UpdateKeyframe();
				} else if (component.KeyframeExists()) {
					component.RemoveKeyframe();
				} else {
					component.AddKeyframe();
				}
			}
			
			find_previous_and_next_keyframes();

			evaluate_timelines();
			
			commit();

		}
	});
	
	// Next Frame Button
	mNextKeyBtn = std::make_shared<nanogui::Button>(*mKeyBtnWrapper, "", FA_STEP_FORWARD);
	mNextKeyBtn->set_enabled(false);
	mNextKeyBtn->set_fixed_width(keyBtnWidth);
	mNextKeyBtn->set_fixed_height(keyBtnHeight);
	mNextKeyBtn->set_tooltip("Next Keyframe");
	mNextKeyBtn->set_callback([this]() {
		stop_playback();
		
		if (!mActiveActor.has_value()) return; // No active actor selected
		
		auto [_, nextKeyframe] = find_previous_and_next_keyframes();

		if (nextKeyframe.mActive) {
			// Update current time to the earliest next keyframe's time
			mCurrentTime = static_cast<int>(nextKeyframe.mTime);
			update_time_display(mCurrentTime);
			mTimelineSlider->set_value(static_cast<float>(mCurrentTime) / mTotalFrames);
			
			refresh_actors();

			// Verify keyframes after time update
			find_previous_and_next_keyframes();

			evaluate_timelines();
			
			commit();

			
		} else {
			// No next keyframe available
			std::cout << "No next keyframe available." << std::endl;
		}
	});
	
	
	mRegistry->RegisterOnActorSelectedCallback(*this);
	
	set_position(nanogui::Vector2i(0, 0));  // Stick to top
}

SceneTimeBar::~SceneTimeBar() {
	mRegistry->UnregisterOnActorSelectedCallback(*this);
}

// Override OnActorSelected from IActorSelectedCallback
void SceneTimeBar::OnActorSelected(std::optional<std::reference_wrapper<Actor>> actor) {
	
	if (actor.has_value()){
		if (actor->get().find_component<TimelineComponent>()) {
			mActiveActor = actor;
		} else {
			mActiveActor = std::nullopt;
		}
	} else {
		mActiveActor = std::nullopt;
	}
	
	stop_playback();
	
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
			find_previous_and_next_keyframes();
		}
	}
	
	evaluate_timelines();
	
	verify_uncommited_key();
	evaluate_keyframe_status();
	
}

void SceneTimeBar::overlay() {
	
	auto ctx = screen().nvg_context();
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
			auto& component = mActiveActor->get().get_component<TimelineComponent>();
			
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
		auto& component = mActiveActor->get().get_component<TimelineComponent>();
		
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

// Helper method to evaluate timelines
void SceneTimeBar::evaluate_timelines() {
	for (auto& animatableActor : mAnimatableActors) {
		auto& component = animatableActor.get().get_component<TimelineComponent>();
		
		if (!mRecording && !mPlaying){
			component.Freeze();
		} else if (!mRecording) {
			component.Unfreeze();
		}
		
		component.Evaluate();
		
		// double Freezing check due to overridal of mProvider
		if (!mRecording) {
			component.Unfreeze();
		}
	}
}

void SceneTimeBar::commit() {
	mAnimationTimeProvider.Update(mCurrentTime);
	
	mUncommittedKey = false;
	
	if (mActiveActor.has_value()) {
		auto& component = mActiveActor->get().get_component<TimelineComponent>();
		
		component.Unfreeze();
		
		component.SyncWithProvider();
	}

}

// Helper method to evaluate keyframe status
void SceneTimeBar::evaluate_keyframe_status() {
	bool atKeyframe = false;
	bool betweenKeyframes = false;
	
	// Loop through animatable actors to determine keyframe status
	if (mActiveActor.has_value()) {
		auto& component = mActiveActor->get().get_component<TimelineComponent>();
		
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
std::tuple<SceneTimeBar::KeyframeStamp, SceneTimeBar::KeyframeStamp> SceneTimeBar::find_previous_and_next_keyframes() {
	// Re-evaluate
	mAnimationTimeProvider.Update(mCurrentTime);
	
	if (!mActiveActor.has_value()) {
		// No active actor; disable both buttons
		mPrevKeyBtn->set_enabled(false);
		mNextKeyBtn->set_enabled(false);
		return std::make_tuple(KeyframeStamp(false, -1.0f), KeyframeStamp(false, -1.0f));
	}
	
	float currentTimeFloat = static_cast<float>(mCurrentTime);
	
	// Get the AnimationComponent
	auto& component = mActiveActor->get().get_component<TimelineComponent>();
	
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
	
	// Enable or disable the Previous Keyframe button
	mPrevKeyBtn->set_enabled(hasPrevKeyframe);
	
	// Enable or disable the Next Keyframe button
	mNextKeyBtn->set_enabled(hasNextKeyframe);
	
	return std::make_tuple(KeyframeStamp(hasPrevKeyframe, latestPrevTime), KeyframeStamp(hasNextKeyframe, earliestNextTime));
}

// Helper method to refresh animatable actors
void SceneTimeBar::refresh_actors() {
	mAnimatableActors = mActorManager.get_actors_with_component<TimelineComponent>();
}

// Helper method to register actor callbacks
void SceneTimeBar::register_actor_callbacks() {
	if (mActiveActor != std::nullopt) {
		auto& component = mActiveActor->get().get_component<TimelineComponent>();
		
		component.TriggerRegistration();
	}
}

void SceneTimeBar::populate_tree(Actor &actor, std::shared_ptr<nanogui::TreeViewItem> parent_node) {
	// Correctly reference the actor's name
	std::shared_ptr<nanogui::TreeViewItem> node =
	parent_node
	? parent_node->add_node(
							std::string{actor.get_component<MetadataComponent>().name()},
							[this, &actor]() {
								
							})
	: mTakeTreeView->add_node(std::string{actor.get_component<MetadataComponent>().name()},
						  [this, &actor]() {
		OnActorSelected(actor);
	});
	
	if (actor.find_component<UiComponent>()) {
		actor.remove_component<UiComponent>();
	}
	
	actor.add_component<UiComponent>([this, &actor, node]() {
		mTakeTreeView->set_selected(node.get());
	});
	
	perform_layout(screen().nvg_context());
}
