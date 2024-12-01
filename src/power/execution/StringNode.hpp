#pragma once

#include "BlueprintNode.hpp"

#include <nanogui/textbox.h>

#include <memory>
#include <string>

class BlueprintCanvas;

class StringCoreNode : public CoreNode {
public:
	StringCoreNode(long long id);
	
	void set_data(const std::string& data);
	
private:
	CorePin& mOutput;
};

class StringVisualNode : public VisualBlueprintNode {
public:
	StringVisualNode(BlueprintCanvas& parent, nanogui::Vector2i position, nanogui::Vector2i size, StringCoreNode& coreNode);
};

