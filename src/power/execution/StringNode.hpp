#pragma once

#include "Node.hpp"

#include <nanogui/textbox.h>
#include <nanogui/vector.h>
#include <nanogui/widget.h>

#include <memory>
#include <string>


namespace blueprint {
class BlueprintCanvas;

class StringNode : public Node {
public:
	StringNode(BlueprintCanvas& parent, const std::string& title, nanogui::Vector2i size, int id, int output_pin_id);
	
private:
	std::unique_ptr<nanogui::TextBox> mTextBox;
};

}
