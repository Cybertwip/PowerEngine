#include "ui/AnimationPanel.hpp"

#include "actors/Actor.hpp"
#include "animation/Animation.hpp"
#include "components/PlaybackComponent.hpp"
#include "filesystem/CompressedSerialization.hpp"

#include <nanogui/icons.h>
#include <nanogui/layout.h>
#include <nanogui/screen.h>
#include <nanogui/toolbutton.h>
#include <nanogui/widget.h>

#include <iostream>

AnimationPanel::AnimationPanel(nanogui::Widget& parent, nanogui::Screen& screen, AnimationTimeProvider& previewTimeProvider)
: Panel(parent, "Animation"), mActiveActor(std::nullopt), mPreviewTimeProvider(previewTimeProvider) {
	set_position(nanogui::Vector2i(0, 0));
	set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Fill, 10, 10));
	
	// Setup for the playback control buttons
	auto playbackLayout = std::make_unique<nanogui::GridLayout>(nanogui::Orientation::Horizontal, 2,
																nanogui::Alignment::Middle, 0, 0);
	
	mPlaybackPanel = std::make_shared<Widget>(std::make_optional<std::reference_wrapper<nanogui::Widget>>(*this));
	
	playbackLayout->set_row_alignment(nanogui::Alignment::Fill);
	mPlaybackPanel->set_layout(std::move(playbackLayout));
	
	// Reverse Play Button
	mReversePlayButton = std::make_shared<nanogui::ToolButton>(*mPlaybackPanel, FA_BACKWARD);
	mReversePlayButton->set_tooltip("Reverse");
	mReversePlayButton->set_change_callback([this](bool active){
		if (mActiveActor.has_value()) {
			auto& playback = mActiveActor->get().get_component<PlaybackComponent>();
			playback.update_state(active ? PlaybackState::Play : PlaybackState::Pause, PlaybackModifier::Reverse, PlaybackTrigger::None, playback.getPlaybackData());
		}
	});
	
	// Forward Play/Pause Button
	mPlayPauseButton = std::make_shared<nanogui::ToolButton>(*mPlaybackPanel, FA_FORWARD);
	mPlayPauseButton->set_tooltip("Play");
	mPlayPauseButton->set_change_callback([this](bool active){
		if (mActiveActor.has_value()) {
			auto& playback = mActiveActor->get().get_component<PlaybackComponent>();
			playback.update_state(active ? PlaybackState::Play : PlaybackState::Pause, PlaybackModifier::Forward, PlaybackTrigger::None, playback.getPlaybackData());
		}
	});
	
	// Initially hide the panel
	set_active_actor(std::nullopt);
}

void AnimationPanel::parse_file(const std::string& path) {
	if (!mActiveActor.has_value()) {
		return;
	}
	
	CompressedSerialization::Deserializer deserializer;
	
	// Load the serialized animation file
	if (!deserializer.load_from_file(path)) {
		std::cerr << "Failed to load serialized file: " << path << "\n";
		return;
	}
	
	auto animation = std::make_unique<Animation>();
	animation->deserialize(deserializer);
	
	auto& playbackComponent = mActiveActor->get().get_component<PlaybackComponent>();
	auto playbackData = std::make_shared<PlaybackData>(std::move(animation));
	
	playbackComponent.setPlaybackData(playbackData);
	
	// Reset button states
	mPlayPauseButton->set_pushed(false);
	mReversePlayButton->set_pushed(false);
}

void AnimationPanel::set_active_actor(std::optional<std::reference_wrapper<Actor>> actor) {
	mActiveActor = actor;
	
	bool should_be_visible = false;
	if (mActiveActor.has_value()) {
		Actor& actorRef = mActiveActor->get();
		
		// Only show the panel if the actor has a component for animation playback
		if (actorRef.find_component<PlaybackComponent>()) {
			should_be_visible = true;
			set_title("Animation");
			
			auto& playback = actorRef.get_component<PlaybackComponent>();
			auto state = playback.get_state();
			
			// Update UI button states to match the component's state
			if (state->getPlaybackState() == PlaybackState::Pause) {
				mPlayPauseButton->set_pushed(false);
				mReversePlayButton->set_pushed(false);
			} else if (state->getPlaybackState() == PlaybackState::Play) {
				bool is_forward = state->getPlaybackModifier() == PlaybackModifier::Forward;
				mPlayPauseButton->set_pushed(is_forward);
				mReversePlayButton->set_pushed(!is_forward);
			}
		}
	}
	
	set_visible(should_be_visible);
	
	// Request a layout update from the parent
	if (parent()) {
		parent()->get().perform_layout(screen().nvg_context());
	}
}

void AnimationPanel::update_with(int currentTime) {
	mPreviewTimeProvider.SetTime(currentTime);
}
