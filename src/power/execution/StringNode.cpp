#include "StringNode.hpp"

namespace blueprint {
StringNode::StringNode(nanogui::Widget& parent, const std::string& title, nanogui::Vector2i size, int id, int output_pin_id)
: Node(parent, title, size, id) {
	
	// Text box inside the node data wrapper: Will be centered due to the vertical alignment
	auto& textbox = add_data_widget<nanogui::TextBox>("");
	
	textbox.set_editable(true);
	textbox.set_alignment(nanogui::TextBox::Alignment::Left);
	
	auto& output = add_output(output_pin_id, this->id, "", PinType::String);
	
	link = [&output](){
		output.can_flow = true;
	};
}
}
