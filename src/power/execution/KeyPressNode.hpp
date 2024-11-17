#pragma once

#include "BlueprintNode.hpp"

#include <nanogui/button.h>

#include <memory>
#include <string>

namespace blueprint {
class BlueprintCanvas;

class KeyPressNode : public BlueprintNode {
public:
	KeyPressNode(std::optional<std::reference_wrapper<BlueprintCanvas>> parent, const std::string& title, nanogui::Vector2i size, int id, int flow_pin_id);
	
private:
	bool keyboard_event(int key, int scancode, int action, int modifiers) override;

	int mKeyCode;
	bool mListening;
	bool mConfigured;
	bool mTriggered;
	
	nanogui::Button& mActionButton;
};

}
