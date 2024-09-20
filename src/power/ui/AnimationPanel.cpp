#include "ui/AnimationPanel.hpp"

#include <nanogui/button.h>
#include <nanogui/icons.h>
#include <nanogui/label.h>
#include <nanogui/layout.h>
#include <nanogui/textbox.h>
#include <nanogui/window.h>

#include "actors/Actor.hpp"
#include "components/TransformComponent.hpp"

AnimationPanel::AnimationPanel(nanogui::Widget &parent)
: Panel(parent, "Animation"), mActiveActor(std::nullopt) {
	set_position(nanogui::Vector2i(0, 0));
	set_layout(new nanogui::GroupLayout());
	
}

AnimationPanel::~AnimationPanel() {
	
}

void AnimationPanel::set_active_actor(std::optional<std::reference_wrapper<Actor>> actor) {

	mActiveActor = actor;
	
	if (mActiveActor.has_value()) {
		
	}
}
