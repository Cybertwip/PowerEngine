#include "NodeProcessor.hpp"

#include "Canvas.hpp"

#include "BlueprintCanvas.hpp"
#include "BlueprintNode.hpp"
#include "KeyPressNode.hpp"
#include "PrintNode.hpp"
#include "StringNode.hpp"

#include <unordered_set>

blueprint::NodeProcessor::NodeProcessor() {
	
}

int blueprint::NodeProcessor::get_next_id() {
	return next_id++;
}

void blueprint::NodeProcessor::build_node(blueprint::BlueprintNode& node){
	node.build();
}

blueprint::Link* blueprint::NodeProcessor::create_link(blueprint::BlueprintCanvas& parent, blueprint::Pin& output, blueprint::Pin& input){
	auto link = std::make_unique<blueprint::Link>(parent, get_next_id(), output, input);
	links.push_back(std::move(link));
	
	return links.back().get();
}

blueprint::BlueprintNode* blueprint::NodeProcessor::spawn_string_node(blueprint::BlueprintCanvas& parent, const nanogui::Vector2i& position) {
	auto node = std::make_unique<blueprint::StringNode>(parent, "String",  nanogui::Vector2i(196, 64), get_next_id(), get_next_id());
	node->set_position(position);
	build_node(*node);
	nodes.push_back(std::move(node));
	return nodes.back().get();
}

blueprint::BlueprintNode* blueprint::NodeProcessor::spawn_print_string_node(blueprint::BlueprintCanvas& parent, const nanogui::Vector2i& position) {
	auto node = std::make_unique<blueprint::PrintNode>(parent, "Print",  nanogui::Vector2i(128, 64), get_next_id(), get_next_id(), get_next_id(), get_next_id());
	node->set_position(position);
	build_node(*node);
	nodes.push_back(std::move(node));
	
	return nodes.back().get();
}

blueprint::BlueprintNode* blueprint::NodeProcessor::spawn_key_press_node(blueprint::BlueprintCanvas& parent, const nanogui::Vector2i& position) {
	auto node = std::make_unique<blueprint::KeyPressNode>(parent, "Key Press",  nanogui::Vector2i(128, 64), get_next_id(), get_next_id());
	node->set_position(position);
	build_node(*node);
	nodes.push_back(std::move(node));
	return nodes.back().get();
}

//
//	Node& spawn_input_action_node(const std::string& key_string, int key_code) {
//	}

void blueprint::NodeProcessor::evaluate() {

	std::unordered_set<blueprint::BlueprintNode*> evaluated_nodes;
	
	for (auto& link : links)
	{
		auto& out_pin = link->get_start();
		auto& in_pin = link->get_end();

		if(evaluated_nodes.find(out_pin.node) == evaluated_nodes.end()){
			if(out_pin.node->evaluate){
				out_pin.node->evaluate();
				
				evaluated_nodes.insert(out_pin.node);
			}
		}

		if(evaluated_nodes.find(in_pin.node) == evaluated_nodes.end()){
			if(in_pin.node->evaluate){
				in_pin.node->evaluate();
				
				evaluated_nodes.insert(in_pin.node);
			}
		}
		
		
		if (in_pin.type == PinType::Flow) {
			bool can_flow = false;  // Initialize to false
			
			for (auto& other_link : links) {
				auto& other_pin = other_link->get_end();
				
				if (&other_pin == &in_pin) {
					auto& other_start_pin = other_link->get_start();
					if (other_start_pin.type == PinType::Flow) {
						can_flow = can_flow || other_start_pin.can_flow; // If any start pin can flow, set canFlow to true
					}
				}
			}
			
			in_pin.can_flow = can_flow;
		}
		
		if (in_pin.can_flow && out_pin.can_flow) {
			in_pin.data = out_pin.data;
		}
	}
	
	
//	for (auto& link : m_Links)
//	{
//		auto outPin = FindPin(link->StartPinID);
//		auto inPin = FindPin(link->EndPinID);
//		
//		inPin->Data = outPin->Data;
//		
//		if(inPin->Type == PinType::Flow && outPin->Type == PinType::Flow){
//			if(outPin->CanFlow && inPin->CanFlow){
//				ed::Flow(link->ID);
//			}
//		} else {
//			if(inPin->CanFlow && outPin->CanFlow){
//				ed::Flow(link->ID);
//			}
//		}
//	}

	
}
