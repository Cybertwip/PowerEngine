#pragma once

#include "BlueprintNode.hpp"

#include <nanogui/textbox.h>

#include <memory>
#include <string>

class BlueprintCanvas;

class StringNode : public BlueprintNode {
public:
	StringNode(std::optional<std::reference_wrapper<BlueprintCanvas>> parent, long long id, nanogui::Vector2i size);
};

