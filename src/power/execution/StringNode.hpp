#pragma once

#include "BlueprintNode.hpp"

#include <nanogui/textbox.h>

#include <memory>
#include <string>

class BlueprintCanvas;

class StringCoreNode : public DataCoreNode {
public:
	StringCoreNode(long long id);

	void set_data(std::optional<std::variant<Entity, std::string, int, float, bool>> data) override;

private:
	std::optional<std::variant<Entity, std::string, int, float, bool>> get_data() override;
	
	CorePin& mOutput;
};

class StringVisualNode : public VisualBlueprintNode {
public:
	StringVisualNode(BlueprintCanvas& parent, nanogui::Vector2i position, nanogui::Vector2i size, StringCoreNode& coreNode);
	
	void perform_layout(NVGcontext *ctx) override {
		VisualBlueprintNode::perform_layout(ctx);
		
		auto data = mCoreNode.get_data();
		
		if (data.has_value()) {
			mTextBox.set_value(std::get<std::string>(data));
		} else {
			mTextBox.set_value("");
		}
	}
	
private:
	nanogui::TextBox& mTextBox;
	StringCoreNode& mCoreNode;
};

