#include "PrintNode.hpp"

blueprint::PrintNode::PrintNode(std::optional<std::reference_wrapper<BlueprintCanvas>> parent, nanogui::Vector2i size, std::function<int()> id_registrator_lambda)
: BlueprintNode(parent, NodeType::Print, "Print", size, id_registrator_lambda(), nanogui::Color(255, 0, 255, 255)) {
	auto& input_flow = add_input(id_registrator_lambda(), this->id, "", PinType::Flow, PinSubType::None);
	auto& input = add_input(id_registrator_lambda(), this->id, "", PinType::String, PinSubType::None);
	auto& output_flow = add_output(id_registrator_lambda(), this->id, "", PinType::Flow, PinSubType::None);
	
	link = [&output_flow](){
		output_flow.can_flow = true;
	};
	
	evaluate = [&input_flow, &input](){
		if(input_flow.can_flow){
			auto data = input.get_data();
			if(data.has_value()){
				std::cout << std::get<std::string>(*data) << std::endl;
			}
		}
		
		input.can_flow = input_flow.can_flow;
	};
}
