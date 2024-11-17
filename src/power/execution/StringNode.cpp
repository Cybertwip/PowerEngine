#include "StringNode.hpp"

blueprint::StringNode::StringNode(std::optional<std::reference_wrapper<BlueprintCanvas>> parent, nanogui::Vector2i size, std::function<int()> id_registrator_lambda)
: BlueprintNode(parent, NodeType::String, "String", size, id_registrator_lambda(), nanogui::Color(255, 0, 255, 255)) {
	
	// Text box inside the node data wrapper: Will be centered due to the vertical alignment
	auto& textbox = add_data_widget<nanogui::TextBox>("");
	
	textbox.set_editable(true);
	textbox.set_alignment(nanogui::TextBox::Alignment::Left);
	
	auto& output = add_output(id_registrator_lambda(), this->id, "", PinType::String);
	
	link = [&output](){
		output.can_flow = true;
	};
	
	evaluate = [&output, &textbox]() {
		output.data = textbox.value();
	};
}
