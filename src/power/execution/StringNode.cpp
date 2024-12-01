#include "StringNode.hpp"

StringCoreNode::StringCoreNode(long long id)
: DataCoreNode(NodeType::String, id, nanogui::Color(255, 0, 255, 255))
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
: VisualBlueprintNode(parent, "String", position, size, coreNode)
, mTextBox(add_data_widget<nanogui::TextBox>(""))
, mCoreNode(coreNode) {
	
	mTextBox.set_placeholder("Enter Text");
	mTextBox.set_editable(true);
	mTextBox.set_alignment(nanogui::TextBox::Alignment::Left);
	
	mTextBox.set_callback([this](const std::string& value) {
		mCoreNode.set_data(value);
		return true;
	});
}
