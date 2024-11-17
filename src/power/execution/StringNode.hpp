#pragma once

#include "BlueprintNode.hpp"

#include <nanogui/textbox.h>

#include <memory>
#include <string>

namespace blueprint {
class BlueprintCanvas;

class StringNode : public BlueprintNode {
public:
	StringNode(std::optional<std::reference_wrapper<BlueprintCanvas>> parent, nanogui::Vector2i size, std::function<int()> id_registrator_lambda);
	
private:
	std::unique_ptr<nanogui::TextBox> mTextBox;
};

}
