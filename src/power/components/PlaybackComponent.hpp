#pragma once

#include "SkinnedAnimationComponent.hpp"

#include <functional>
#include <unordered_map>

class PlaybackComponent {
public:
	using PlaybackChangedCallback = std::function<void(const PlaybackComponent&)>;
		
	PlaybackComponent()
	:
	mState(0,
		   SkinnedAnimationComponent::PlaybackState::Pause,
		   SkinnedAnimationComponent::PlaybackModifier::Forward,
		   SkinnedAnimationComponent::PlaybackTrigger::None) {
		
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
	SkinnedAnimationComponent::PlaybackState getPlaybackState() const {
		return mState.getPlaybackState();
	}
	
	// Setter for PlaybackState
	void setPlaybackState(SkinnedAnimationComponent::PlaybackState state) {
		mState.setPlaybackState(state);
		trigger_on_playback_changed();
	}
	
	// Getter for PlaybackModifier
	SkinnedAnimationComponent::PlaybackModifier getPlaybackModifier() const {
		return mState.getPlaybackModifier();
	}
	
	// Setter for PlaybackModifier
	void setPlaybackModifier(SkinnedAnimationComponent::PlaybackModifier modifier) {
		mState.setPlaybackModifier(modifier);
		trigger_on_playback_changed();
	}
	
	// Getter for PlaybackTrigger
	SkinnedAnimationComponent::PlaybackTrigger getPlaybackTrigger() const {
		return mState.getPlaybackTrigger();
	}
	
	// Setter for PlaybackTrigger
	void setPlaybackTrigger(SkinnedAnimationComponent::PlaybackTrigger trigger) {
		mState.setPlaybackTrigger(trigger);
		trigger_on_playback_changed();
	}
	
	const SkinnedAnimationComponent::Keyframe& get_state() const {
		return mState;
	}
	
	void update_state(SkinnedAnimationComponent::PlaybackState state, SkinnedAnimationComponent::PlaybackModifier modifier, SkinnedAnimationComponent::PlaybackTrigger trigger) {
		mState.setPlaybackState(state);

		mState.setPlaybackModifier(modifier);

		mState.setPlaybackTrigger(trigger);
		
		trigger_on_playback_changed();
	}
	
private:
	SkinnedAnimationComponent::Keyframe mState;

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
