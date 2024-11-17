#pragma once

#include "BlueprintNode.hpp"

#include <nanogui/textbox.h>

#include <memory>
#include <string>

namespace blueprint {
class BlueprintCanvas;

class KeyReleaseNode : public BlueprintNode {
public:
	KeyReleaseNode(BlueprintCanvas& parent, const std::string& title, nanogui::Vector2i size, int id, int flow_pin_id);
	
private:
	bool keyboard_event(int key, int scancode, int action, int modifiers) override;
	
	int mKeyCode;
	bool mListening;
	bool mConfigured;
	bool mTriggered;
	std::unique_ptr<nanogui::Button> mButton;
};

}
