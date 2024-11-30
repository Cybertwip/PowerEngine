#pragma once

#include "BlueprintNode.hpp"

#include <nanogui/button.h>

#include <memory>
#include <optional>
#include <string>
#include <variant>

class BlueprintCanvas;

class KeyPressNode : public BlueprintDataNode {
public:
	KeyPressNode(std::optional<std::reference_wrapper<BlueprintCanvas>> parent, long long id, nanogui::Vector2i size, std::function<int()> id_registrator_lambda);

private:
	std::optional<std::variant<Entity, std::string, int, float, bool>> get_data() override {
		return mKeyCode;
	}
	
	void set_data(std::optional<std::variant<Entity, std::string, int, float, bool>> data) override;
	
	bool keyboard_event(int key, int scancode, int action, int modifiers) override;

	int mKeyCode;
	bool mListening;
	bool mConfigured;
	bool mTriggered;
	
	nanogui::Button& mActionButton;
};

