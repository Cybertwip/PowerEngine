#pragma once

#include "Panel.hpp"
#include "animation/AnimationTimeProvider.hpp"

#include <optional>
#include <string>

// Forward declarations
class Actor;

namespace nanogui {
class Screen;
class ToolButton;
class Widget;
}

class AnimationPanel : public Panel {
public:
	AnimationPanel(nanogui::Widget& parent, nanogui::Screen& screen);
	
	// Sets the currently focused actor to control its animations
	void set_active_actor(std::optional<std::reference_wrapper<Actor>> actor);
	
	// Loads an animation file for the active actor
	void parse_file(const std::string& path);
		
private:	
	// The actor whose animations are being controlled
	std::optional<std::reference_wrapper<Actor>> mActiveActor;
	
	// UI Elements for playback control
	std::shared_ptr<nanogui::ToolButton> mReversePlayButton;
	std::shared_ptr<nanogui::ToolButton> mPlayPauseButton;
	std::shared_ptr<nanogui::Widget> mPlaybackPanel;
};
