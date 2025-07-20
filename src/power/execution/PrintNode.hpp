#pragma once

#include "BlueprintNode.hpp"

#include <nanogui/textbox.h>

#include <memory>
#include <string>

class BlueprintCanvas;

class PrintCoreNode : public CoreNode {
public:
	PrintCoreNode(UUID id);
};

class PrintVisualNode : public VisualBlueprintNode {
public:
	PrintVisualNode(BlueprintCanvas& parent, nanogui::Vector2i position, nanogui::Vector2i size, PrintCoreNode& coreNode);
};

