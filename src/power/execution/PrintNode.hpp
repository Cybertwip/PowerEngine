#pragma once

#include "BlueprintNode.hpp"

#include <nanogui/textbox.h>

#include <memory>
#include <string>

class BlueprintCanvas;

class PrintNode : public BlueprintNode {
public:
	PrintNode(std::optional<std::reference_wrapper<BlueprintCanvas>> parent, long long id, nanogui::Vector2i size, std::function<int()> id_registrator_lambda);
};

