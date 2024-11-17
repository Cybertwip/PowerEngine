#pragma once

#include "BlueprintNode.hpp"

#include <nanogui/textbox.h>

#include <memory>
#include <string>

namespace blueprint {
class BlueprintCanvas;

class PrintNode : public BlueprintNode {
public:
	PrintNode(std::optional<std::reference_wrapper<BlueprintCanvas>> parent, const std::string& title, nanogui::Vector2i size, int id, int flow_pin_id, int input_pin_id, int output_pin_id);
	
private:
	std::unique_ptr<nanogui::TextBox> mTextBox;
};

}
