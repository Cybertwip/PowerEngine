#include "PrintNode.hpp"

#include <iostream>

PrintNode::PrintNode(std::optional<std::reference_wrapper<BlueprintCanvas>> parent, long long id, nanogui::Vector2i size)
: BlueprintNode(parent, NodeType::Print, "Print", size, id, nanogui::Color(255, 0, 255, 255)) {
	auto& input_flow = add_input(this->id, "", PinType::Flow, PinSubType::None);
	auto& input = add_input(this->id, "", PinType::String, PinSubType::None);
	auto& output_flow = add_output(this->id, "", PinType::Flow, PinSubType::None);
	
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

PrintCoreNode::PrintCoreNode(long long id)
: CoreNode(NodeType::Print, id, nanogui::Color(255, 0, 255, 255)) {
	
	auto& input_flow = add_input(PinType::Flow, PinSubType::None);
	auto& input = add_input(PinType::String, PinSubType::None);
	auto& output_flow = add_output(PinType::Flow, PinSubType::None);
	
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

PrintVisualNode::PrintVisualNode(BlueprintCanvas& parent, nanogui::Vector2i position, nanogui::Vector2i size, PrintCoreNode& coreNode)
: VisualBlueprintNode(parent, "Print", position, size, coreNode) {
	
}
