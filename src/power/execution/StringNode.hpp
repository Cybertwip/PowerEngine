#pragma once

#include "BlueprintNode.hpp"

#include <nanogui/textbox.h>

#include <memory>
#include <string>

class BlueprintCanvas;

class StringCoreNode : public DataCoreNode {
public:
	StringCoreNode(long long id);

private:
	std::optional<std::variant<Entity, std::string, int, float, bool>> get_data() override;
	
	void set_data(std::optional<std::variant<Entity, std::string, int, float, bool>> data) override;
	
	CorePin& mOutput;
};

class StringVisualNode : public VisualBlueprintNode {
public:
	StringVisualNode(BlueprintCanvas& parent, nanogui::Vector2i position, nanogui::Vector2i size, StringCoreNode& coreNode);
};

