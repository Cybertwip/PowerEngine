#include "ui/AnimationPanel.hpp"

#include <nanogui/toolbutton.h>
#include <nanogui/icons.h>
#include <nanogui/layout.h>

#include "actors/Actor.hpp"
#include "components/TransformComponent.hpp"

AnimationPanel::AnimationPanel(nanogui::Widget &parent)
: Panel(parent, "Animation"), mActiveActor(std::nullopt) {
	set_position(nanogui::Vector2i(0, 0));
	set_layout(new nanogui::GroupLayout());

	auto *playbackLayout = new nanogui::GridLayout(nanogui::Orientation::Horizontal, 2,
													nanogui::Alignment::Middle, 0, 0);
	
	Widget *playbackPanel = new Widget(this);

	playbackLayout->set_row_alignment(nanogui::Alignment::Fill);
	
	playbackPanel->set_layout(playbackLayout);

	mReversePlayButton = new nanogui::ToolButton(playbackPanel, FA_BACKWARD);
	mReversePlayButton->set_tooltip("Reverse");

	mPlayPauseButton = new nanogui::ToolButton(playbackPanel, FA_FORWARD);
	mPlayPauseButton->set_tooltip("Play");
}

AnimationPanel::~AnimationPanel() {
	
}

void AnimationPanel::set_active_actor(std::optional<std::reference_wrapper<Actor>> actor) {

	mActiveActor = actor;
	
	if (mActiveActor.has_value()) {
		
	}
}
