#pragma once

#include "Node.hpp"

#include <nanogui/textbox.h>
#include <nanogui/vector.h>
#include <nanogui/widget.h>

#include <memory>
#include <string>

namespace blueprint {
class BlueprintCanvas;

class PrintNode : public Node {
public:
	PrintNode(BlueprintCanvas& parent, const std::string& title, nanogui::Vector2i size, int id, int flow_pin_id, int input_pin_id, int output_pin_id);
	
private:
	std::unique_ptr<nanogui::TextBox> mTextBox;
};

}
