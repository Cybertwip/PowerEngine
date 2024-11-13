#pragma once

#include "actors/IActorSelectedCallback.hpp"

#include <optional>
#include <functional>
#include <vector>
#include <glm/glm.hpp>

#include <nanogui/nanogui.h>
#include <nanogui/treeview.h>
#include <nanogui/vscrollpanel.h>


// Forward declarations to minimize dependencies
class ActorManager;
class Actor;
class AnimationComponent;
class AnimationTimeProvider;
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
	struct KeyframeStamp {
		bool mActive;
		float mTime;
		
		KeyframeStamp(bool active, float time) : mActive(active), mTime(time) {
			
		}
	};
	
public:
	SceneTimeBar(nanogui::Widget& parent, ActorManager& actorManager, AnimationTimeProvider& animationTimeProvider, std::shared_ptr<IActorSelectedRegistry> registry, int width, int height);
	~SceneTimeBar();
	
	// Override OnActorSelected from IActorSelectedCallback
	void OnActorSelected(std::optional<std::reference_wrapper<Actor>> actor) override;
	
	void stop_playback();

	// Method to update the SceneTimeBar
	void update();
	
	void overlay();
	
	void toggle_play_pause(bool play);

	// Getter for current time
	int current_time() const;
	
	void refresh_actors();
	
	bool is_playing() const {
		return mPlaying;
	}

private:
	void verify_uncommited_key();
	
	// Helper methods
	void update_time_display(int frameCount);
	void evaluate_timelines();
	void commit();
	void evaluate_keyframe_status();
	std::tuple<KeyframeStamp, KeyframeStamp> find_previous_and_next_keyframes();
	
	void register_actor_callbacks();
	
	void populate_tree(Actor& actor, std::shared_ptr<nanogui::TreeViewItem> parentNode = nullptr);
	
	// Override mouse events to consume them
	bool mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;
	bool mouse_motion_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override;
	
private:
	// Member variables
	ActorManager& mActorManager;
	AnimationTimeProvider& mAnimationTimeProvider;
	std::shared_ptr<IActorSelectedRegistry> mRegistry;
	std::shared_ptr<nanogui::TextBox> mTimeLabel;
	std::shared_ptr<nanogui::Slider> mTimelineSlider;
	std::shared_ptr<nanogui::Button> mRecordBtn;
	std::shared_ptr<nanogui::ToolButton> mPlayPauseBtn;
	std::shared_ptr<nanogui::Button> mKeyBtn;
	std::shared_ptr<nanogui::Button> mPrevKeyBtn;
	std::shared_ptr<nanogui::Button> mNextKeyBtn;
	std::shared_ptr<nanogui::Widget> mSliderWrapper;

	std::shared_ptr<nanogui::Widget> mButtonWrapper;
	
	std::shared_ptr<nanogui::TreeView> mTakeTreeView;
	
	std::shared_ptr<nanogui::Button> mRewindBtn;
	std::shared_ptr<nanogui::Button> mSeekEndBtn;
	
	std::shared_ptr<nanogui::Button> mNextFrameBtn;
	std::shared_ptr<nanogui::Button> mPrevFrameBtn;
	
	std::shared_ptr<nanogui::Button> mStopBtn;

	std::shared_ptr<nanogui::Widget> mKeyBtnWrapper;
	
	std::shared_ptr<nanogui::Widget> mButtonWrapperWrapper;
	
	nanogui::Color mBackgroundColor;
	nanogui::Color mNormalButtonColor;
	
	std::vector<std::reference_wrapper<Actor>> mAnimatableActors;
	std::optional<std::reference_wrapper<Actor>> mActiveActor;
	
	bool mRecording;
	bool mPlaying;
	bool mUncommittedKey;
	
	int mCurrentTime;
	int mTotalFrames;
};
