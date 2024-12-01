#pragma once

#include "BlueprintNode.hpp"

#include <nanogui/textbox.h>

#include <memory>
#include <string>

class BlueprintCanvas;

class PrintNode : public BlueprintNode {
public:
	PrintNode(std::optional<std::reference_wrapper<BlueprintCanvas>> parent, long long id, nanogui::Vector2i size);
};


class PrintCoreNode : public CoreNode {
public:
	PrintCoreNode(long long id);
};

class PrintVisualNode : public VisualBlueprintNode {
public:
	PrintVisualNode(BlueprintCanvas& parent, nanogui::Vector2i position, nanogui::Vector2i size, PrintCoreNode& coreNode);
};

