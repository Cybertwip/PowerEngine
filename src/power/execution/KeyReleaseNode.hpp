#pragma once

#include "BlueprintNode.hpp"

#include <nanogui/button.h>

#include <memory>
#include <optional>
#include <string>
#include <variant>

namespace blueprint {
class BlueprintCanvas;

class KeyReleaseNode : public BlueprintNode {
public:
	KeyReleaseNode(std::optional<std::reference_wrapper<BlueprintCanvas>> parent, nanogui::Vector2i size, std::function<int()> id_registrator_lambda);
	
private:
	std::optional<std::variant<Entity, std::string, int, float, bool>> get_data() override {
		return mKeyCode;
	}
	
	void set_data(std::optional<std::variant<Entity, std::string, int, float, bool>> data) override {
		if (data.has_value()) {
			mKeyCode = std::get<int>(data.value());
			auto* caption = glfwGetKeyName(mKeyCode, -1);
			
			if (caption != nullptr) {
				mActionButton.set_caption(caption);
			}

			mConfigured = true;
			
		} else {
			mKeyCode = -1;
			mConfigured = false;
		}
	}
	
	bool keyboard_event(int key, int scancode, int action, int modifiers) override;
	
	int mKeyCode;
	bool mListening;
	bool mConfigured;
	bool mTriggered;
	
	nanogui::Button& mActionButton;

};

}
