#include "StringNode.hpp"

StringCoreNode::StringCoreNode(long long id)
: CoreNode(NodeType::String, id, nanogui::Color(255, 0, 255, 255))
, mOutput(add_output(PinType::String, PinSubType::None)) {
	
	link = [this](){
		mOutput.can_flow = true;
	};
	
}

std::optional<std::variant<Entity, std::string, int, float, bool>> StringCoreNode::get_data() {
	return mOutput.get_data();
}

void StringCoreNode::set_data(std::optional<std::variant<Entity, std::string, int, float, bool>> data) {
	mOutput.set_data(data);
}


StringVisualNode::StringVisualNode(BlueprintCanvas& parent, nanogui::Vector2i position, nanogui::Vector2i size, StringCoreNode& coreNode)
: VisualBlueprintNode(parent, "String", position, size, coreNode) {
	
	auto& textbox = add_data_widget<nanogui::TextBox>("");
	
	textbox.set_editable(true);
	textbox.set_alignment(nanogui::TextBox::Alignment::Left);
	
	textbox.set_callback([&coreNode](const std::string& value) {
		coreNode.set_data(value);
		return true;
	});
	
}
