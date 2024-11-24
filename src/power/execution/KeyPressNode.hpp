#pragma once

#include "BlueprintNode.hpp"

#include <nanogui/button.h>

#include <memory>
#include <optional>
#include <string>
#include <variant>

namespace blueprint {
class BlueprintCanvas;

class KeyPressNode : public BlueprintNode {
public:
	KeyPressNode(std::optional<std::reference_wrapper<BlueprintCanvas>> parent, nanogui::Vector2i size, std::function<int()> id_registrator_lambda);
	
	virtual std::optional<std::variant<Entity, std::string, int, float, bool>> get_data() {
		return mKeyCode;
	}
	
	virtual void set_data(std::optional<std::variant<Entity, std::string, int, float, bool>> data) {
		if (data.has_value()) {
			mKeyCode = std::get<int>(data.value());
			mConfigured = true;
		} else {
			mKeyCode = -1;
			mConfigured = false;
		}
	}

private:
	bool keyboard_event(int key, int scancode, int action, int modifiers) override;

	int mKeyCode;
	bool mListening;
	bool mConfigured;
	bool mTriggered;
	
	nanogui::Button& mActionButton;
};

}
