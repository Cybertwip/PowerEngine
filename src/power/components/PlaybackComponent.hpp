#pragma once

#include <functional>
#include <unordered_map>

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

class PlaybackComponent {
public:
	struct Keyframe {
		float time; // Keyframe time
	private:
		PlaybackState mPlaybackState;
		PlaybackModifier mPlaybackModifier;
		PlaybackTrigger mPlaybackTrigger;
		
	public:
		// Constructors
		Keyframe(float t, PlaybackState state, PlaybackModifier modifier, PlaybackTrigger trigger)
		: time(t), mPlaybackState(state), mPlaybackModifier(modifier), mPlaybackTrigger(trigger) {
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
		void setPlaybackState(PlaybackState state) {
			mPlaybackState = state;
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
		
		// Setter for PlaybackTrigger
		void setPlaybackTrigger(PlaybackTrigger trigger) {
			mPlaybackTrigger = trigger;
			// Setting the trigger should also set PlaybackState to Pause
			if (mPlaybackTrigger != PlaybackTrigger::None) {
				mPlaybackState = PlaybackState::Pause;
			}
		}
		
		// Overloaded == operator for Keyframe
		bool operator==(const Keyframe& rhs) {
			return time == rhs.time &&
			getPlaybackState() == rhs.getPlaybackState() &&
			getPlaybackModifier() == rhs.getPlaybackModifier() &&
			getPlaybackTrigger() == rhs.getPlaybackTrigger();
		}
		
		// Overloaded != operator for Keyframe
		bool operator!=(const Keyframe& rhs) {
			return !(*this == rhs);
		}
	};
public:
	using PlaybackChangedCallback = std::function<void(const PlaybackComponent&)>;
		
	PlaybackComponent()
	:
	mState(0,
		   PlaybackState::Pause,
		   PlaybackModifier::Forward,
		   PlaybackTrigger::None) {
		
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
		return mState.getPlaybackState();
	}
	
	// Setter for PlaybackState
	void setPlaybackState(PlaybackState state) {
		mState.setPlaybackState(state);
		trigger_on_playback_changed();
	}
	
	// Getter for PlaybackModifier
	PlaybackModifier getPlaybackModifier() const {
		return mState.getPlaybackModifier();
	}
	
	// Setter for PlaybackModifier
	void setPlaybackModifier(PlaybackModifier modifier) {
		mState.setPlaybackModifier(modifier);
		trigger_on_playback_changed();
	}
	
	// Getter for PlaybackTrigger
	PlaybackTrigger getPlaybackTrigger() const {
		return mState.getPlaybackTrigger();
	}
	
	// Setter for PlaybackTrigger
	void setPlaybackTrigger(PlaybackTrigger trigger) {
		mState.setPlaybackTrigger(trigger);
		trigger_on_playback_changed();
	}
	
	const Keyframe& get_state() const {
		return mState;
	}
	
	void update_state(PlaybackState state, PlaybackModifier modifier, PlaybackTrigger trigger) {
		mState.setPlaybackState(state);

		mState.setPlaybackModifier(modifier);

		mState.setPlaybackTrigger(trigger);
		
		trigger_on_playback_changed();
	}
	
private:
	Keyframe mState;

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
