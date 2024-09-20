#pragma once

#include "Panel.hpp"

#include <nanogui/textbox.h>

class Actor;
class TransformComponent;

class AnimationPanel : public Panel {
public:
	AnimationPanel(nanogui::Widget& parent);
	~AnimationPanel();

    void set_active_actor(std::optional<std::reference_wrapper<Actor>> actor);
    
private:
    std::optional<std::reference_wrapper<Actor>> mActiveActor;
};
