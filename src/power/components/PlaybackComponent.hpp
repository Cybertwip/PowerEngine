#pragma once

#include "components/AnimationComponent.hpp"

#include <functional>
#include <memory>
#include <vector>
#include <unordered_map>

class Animation;

struct PlaybackData {
	PlaybackData(std::unique_ptr<Animation> animation) : mAnimation(std::move(animation)) {
		
		assert(mAnimation != nullptr);
	}
	
	void set_animation(std::unique_ptr<Animation> animation) {
		assert(animation != nullptr);
		

		mAnimation = std::move(animation);
	}
	
	Animation& get_animation() {
		return *mAnimation;
	}	
	
private:
	PlaybackData(){
		
	}
	std::unique_ptr<Animation> mAnimation;
};

enum class PlaybackState {
	Play,
	Pause
};

enum class PlaybackModifier {
	Forward,
	Reverse
};

enum class PlaybackTrigger {
	None,
	Rewind,
	FastForward
};

class PlaybackComponent : public AnimationComponent {
public:
	
	struct Keyframe {
		float time; // Keyframe time
	private:
		PlaybackState mPlaybackState;
		PlaybackModifier mPlaybackModifier;
		PlaybackTrigger mPlaybackTrigger;
		std::shared_ptr<PlaybackData> mPlaybackData;
		
	public:
		// Constructors
		Keyframe(float t, PlaybackState state, PlaybackModifier modifier, PlaybackTrigger trigger, std::shared_ptr<PlaybackData> playbackData)
		: time(t), mPlaybackState(state), mPlaybackModifier(modifier), mPlaybackTrigger(trigger),
		mPlaybackData(playbackData) {
			assert(mPlaybackData != nullptr);
			// Setting trigger sets state to Pause
			if (trigger == PlaybackTrigger::Rewind || trigger == PlaybackTrigger::FastForward) {
				mPlaybackState = PlaybackState::Pause;
			}
		}
		
		// Getter for PlaybackState
		PlaybackState getPlaybackState() const {
			return mPlaybackState;
		}
		
		// Setter for PlaybackState
		void setPlaybackState(std::reference_wrapper<PlaybackState> state) {
			mPlaybackState = state.get();
		}
		
		// Getter for PlaybackModifier
		PlaybackModifier getPlaybackModifier() const {
			return mPlaybackModifier;
		}
		
		// Setter for PlaybackModifier
		void setPlaybackModifier(PlaybackModifier modifier) {
			mPlaybackModifier = modifier;
		}
		
		// Getter for PlaybackTrigger
		PlaybackTrigger getPlaybackTrigger() const {
			return mPlaybackTrigger;
		}
		
		// Getter for PlaybackData
		std::shared_ptr<PlaybackData> getPlaybackData() const {
			return mPlaybackData;
		}
		
		void setPlaybackData(std::shared_ptr<PlaybackData> playbackData) {
			assert(playbackData != nullptr);
			mPlaybackData = playbackData;
		}
		
		// Setter for PlaybackTrigger
		void setPlaybackTrigger(PlaybackTrigger trigger) {
			mPlaybackTrigger = trigger;
			// Setting the trigger should also set PlaybackState to Pause
			if (mPlaybackTrigger != PlaybackTrigger::None) {
				mPlaybackState = PlaybackState::Pause;
			}
		}
		
		// Overloaded == operator for Keyframe
		bool operator==(const Keyframe& rhs) const {
			return time == rhs.time &&
			getPlaybackState() == rhs.getPlaybackState() &&
			getPlaybackModifier() == rhs.getPlaybackModifier() &&
			getPlaybackTrigger() == rhs.getPlaybackTrigger() &&
			getPlaybackData().get() == rhs.getPlaybackData().get();
		}
		
		// Overloaded != operator for Keyframe
		bool operator!=(const Keyframe& rhs) const {
			return !(*this == rhs);
		}
		
	private:
		Keyframe() {
		}
	};
public:
	using PlaybackChangedCallback = std::function<void(const PlaybackComponent&)>;
		
	PlaybackComponent(std::shared_ptr<Keyframe> state)
	: mState(state)
	{
	}
	
	
	// Callback registration, returns an ID for future unregistration
	int register_on_playback_changed_callback(PlaybackChangedCallback callback) {
		int id = nextCallbackId++;
		callbacks[id] = callback;
		return id;
	}
	
	// Unregister a callback using its ID
	void unregister_on_playback_changed_callback(int id) {
		callbacks.erase(id);
	}
	
	// Getter for PlaybackState
	PlaybackState getPlaybackState() const {
		return mState->getPlaybackState();
	}
	
	// Setter for PlaybackState
	void setPlaybackState(PlaybackState state) {
		mState->setPlaybackState(state);
		trigger_on_playback_changed();
	}
	
	// Getter for PlaybackModifier
	PlaybackModifier getPlaybackModifier() const {
		return mState->getPlaybackModifier();
	}
	
	// Setter for PlaybackModifier
	void setPlaybackModifier(PlaybackModifier modifier) {
		mState->setPlaybackModifier(modifier);
		trigger_on_playback_changed();
	}
	
	// Getter for PlaybackTrigger
	PlaybackTrigger getPlaybackTrigger() const {
		return mState->getPlaybackTrigger();
	}
	
	// Setter for PlaybackTrigger
	void setPlaybackTrigger(PlaybackTrigger trigger) {
		mState->setPlaybackTrigger(trigger);
		trigger_on_playback_changed();
	}
	
	std::shared_ptr<Keyframe> get_state() const {
		return mState;
	}

	void setPlaybackData(std::shared_ptr<PlaybackData> playbackData) {
		assert(playbackData != nullptr);
		mState->setPlaybackData(playbackData);
		trigger_on_playback_changed();
	}

	// Getter for PlaybackTrigger
	std::shared_ptr<PlaybackData> getPlaybackData() const {
		return mState->getPlaybackData();
	}

	Animation& get_animation() const {
		return mState->getPlaybackData()->get_animation();
	}
	
	void update_state(PlaybackState state, PlaybackModifier modifier, PlaybackTrigger trigger, std::shared_ptr<PlaybackData> playbackData) {
		mState->setPlaybackState(state);

		mState->setPlaybackModifier(modifier);

		mState->setPlaybackTrigger(trigger);

		mState->setPlaybackData(playbackData);

		trigger_on_playback_changed();
	}
	
protected:
	PlaybackComponent() = default;
	
	std::shared_ptr<Keyframe> mState;

	// Use an unordered_map to store callbacks with an integer key (ID)
	std::unordered_map<int, PlaybackChangedCallback> callbacks;
	int nextCallbackId = 0;
	
	// Trigger all callbacks when the transform changes
	void trigger_on_playback_changed() {
		for (const auto& [id, callback] : callbacks) {
			callback(*this);
		}
	}
};
