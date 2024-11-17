#include "PrintNode.hpp"

blueprint::PrintNode::PrintNode(std::optional<std::reference_wrapper<BlueprintCanvas>> parent, const std::string& title, nanogui::Vector2i size, int id, int flow_pin_id, int input_pin_id, int output_pin_id)
: BlueprintNode(parent, NodeType::Print, title, size, id, nanogui::Color(255, 0, 255, 255)) {
	auto& input_flow = add_input(flow_pin_id, this->id, "", PinType::Flow);
	auto& input = add_input(input_pin_id, this->id, "", PinType::String);
	auto& output_flow = add_output(output_pin_id, this->id, "", PinType::Flow);
	
	link = [&output_flow](){
		output_flow.can_flow = true;
	};
	
	evaluate = [&input_flow, &input](){
		if(input_flow.can_flow){
			auto& data = input.data;
			if(data.has_value()){
				std::cout << std::get<std::string>(*data) << std::endl;
			}
		}
		
		input.can_flow = input_flow.can_flow;
	};
}
