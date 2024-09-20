#pragma once

#include "Panel.hpp"

namespace nanogui {
class ToolButton;
class Widget;
}

class Actor;
class TransformComponent;

class AnimationPanel : public Panel {
public:
	AnimationPanel(nanogui::Widget& parent);
	~AnimationPanel();

    void set_active_actor(std::optional<std::reference_wrapper<Actor>> actor);
    
private:
    std::optional<std::reference_wrapper<Actor>> mActiveActor;
	
	nanogui::ToolButton* mReversePlayButton; // New reverse play button
	nanogui::ToolButton* mPlayPauseButton;

};
