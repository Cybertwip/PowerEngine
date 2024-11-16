#pragma once

#include "Node.hpp"

#include <nanogui/textbox.h>
#include <nanogui/vector.h>
#include <nanogui/widget.h>

#include <memory>
#include <string>


namespace blueprint {


class StringNode : public Node {
public:
	StringNode(nanogui::Widget& parent, const std::string& title, nanogui::Vector2i size, int id, int output_pin_id);
	
private:
	std::unique_ptr<nanogui::TextBox> mTextBox;
};

}
