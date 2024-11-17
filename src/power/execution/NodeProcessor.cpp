#include "NodeProcessor.hpp"

#include "Canvas.hpp"

#include "BlueprintCanvas.hpp"
#include "BlueprintNode.hpp"
#include "KeyPressNode.hpp"
#include "PrintNode.hpp"
#include "StringNode.hpp"

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

void blueprint::NodeProcessor::evaluate(blueprint::BlueprintNode& node) {
	if (node.evaluate) {
		node.evaluate();
	}
}
