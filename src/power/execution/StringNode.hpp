#pragma once

#include "BlueprintNode.hpp"

#include <nanogui/textbox.h>

#include <memory>
#include <string>

namespace blueprint {
class BlueprintCanvas;

class StringNode : public BlueprintNode {
public:
	StringNode(BlueprintCanvas& parent, const std::string& title, nanogui::Vector2i size, int id, int output_pin_id);
	
private:
	std::unique_ptr<nanogui::TextBox> mTextBox;
};

}
