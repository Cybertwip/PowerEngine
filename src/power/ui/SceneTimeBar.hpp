#pragma once

#include "actors/IActorSelectedCallback.hpp"

#include <optional>
#include <functional>
#include <vector>
#include <glm/glm.hpp>
#include <nanogui/nanogui.h>

// Forward declarations to minimize dependencies
class ActorManager;
class Actor;
class AnimationComponent;
class SkinnedAnimationComponent;
class TransformComponent;
class PlaybackComponent;
class IActorSelectedCallback;
class IActorSelectedRegistry;

namespace nanogui {
class Slider;
class Button;
class ToolButton;
class TextBox;
class Widget;
}

// SceneTimeBar Class Declaration
class SceneTimeBar : public nanogui::Widget, public IActorSelectedCallback {
public:
	SceneTimeBar(nanogui::Widget* parent, ActorManager& actorManager, IActorSelectedRegistry& registry, int width, int height);
	~SceneTimeBar();
	
	// Override OnActorSelected from IActorSelectedCallback
	void OnActorSelected(std::optional<std::reference_wrapper<Actor>> actor) override;
	
	// Method to update the SceneTimeBar
	void update();
	
	// Getter for current time
	int current_time() const;
	
	void refresh_actors();

private:
	// Helper methods
	void toggle_play_pause(bool play);
	void stop_playback();
	void update_time_display(int frameCount);
	void evaluate_transforms();
	void evaluate_animations();
	void evaluate_keyframe_status();
	void verify_previous_next_keyframes(const std::optional<std::reference_wrapper<Actor>>& activeActor);
	void register_actor_callbacks(std::optional<std::reference_wrapper<Actor>> actor);
	
	// Override mouse events to consume them
	bool mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;
	bool mouse_motion_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override;
	
private:
	// Member variables
	ActorManager& mActorManager;
	IActorSelectedRegistry& mRegistry;
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
	std::optional<std::reference_wrapper<PlaybackComponent>> mRegisteredPlaybackComponent;
	int mTransformRegistrationId;
	int mPlaybackRegistrationId;
	int mCurrentTime;
	int mTotalFrames;
};
