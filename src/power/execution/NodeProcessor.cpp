#include "NodeProcessor.hpp"

#include "Canvas.hpp"

#include "actors/Actor.hpp"

#include "BlueprintCanvas.hpp"
#include "BlueprintNode.hpp"
#include "KeyPressNode.hpp"
#include "KeyReleaseNode.hpp"
#include "PrintNode.hpp"
#include "StringNode.hpp"

#include <unordered_set>

class BlueprintComponent {
public:
	BlueprintComponent(std::unique_ptr<blueprint::NodeProcessor> nodeProcessor)
	: mNodeProcessor(std::move(nodeProcessor)) {
		
	}
	
	const blueprint::NodeProcessor& node_processor() {
		return *mNodeProcessor;
	}
	
private:
	std::unique_ptr<blueprint::NodeProcessor> mNodeProcessor;
};

blueprint::NodeProcessor::NodeProcessor() {
	
}

long long blueprint::NodeProcessor::get_next_id() {
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
	auto node = std::make_unique<blueprint::StringNode>(parent, "String",  nanogui::Vector2i(196, 64), [this](){
		return get_next_id();
	});
	node->set_position(position);
	build_node(*node);
	nodes.push_back(std::move(node));
	return nodes.back().get();
}

blueprint::BlueprintNode* blueprint::NodeProcessor::spawn_print_string_node(blueprint::BlueprintCanvas& parent, const nanogui::Vector2i& position) {
	auto node = std::make_unique<blueprint::PrintNode>(parent, "Print",  nanogui::Vector2i(128, 64), [this](){
		return get_next_id();
	});
	node->set_position(position);
	build_node(*node);
	nodes.push_back(std::move(node));
	return nodes.back().get();
}

blueprint::BlueprintNode* blueprint::NodeProcessor::spawn_key_press_node(blueprint::BlueprintCanvas& parent, const nanogui::Vector2i& position) {
	auto node = std::make_unique<blueprint::KeyPressNode>(parent, "Key Press",  nanogui::Vector2i(136, 64), [this](){
		return get_next_id();
	});
	node->set_position(position);
	build_node(*node);
	nodes.push_back(std::move(node));
	return nodes.back().get();
}

blueprint::BlueprintNode* blueprint::NodeProcessor::spawn_key_release_node(blueprint::BlueprintCanvas& parent, const nanogui::Vector2i& position) {
	auto node = std::make_unique<blueprint::KeyReleaseNode>(parent, "Key Release",  nanogui::Vector2i(136, 64), [this](){
		return get_next_id();
	});
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
		
		in_pin.data = out_pin.data;
	}
}

void blueprint::NodeProcessor::serialize(Actor& actor) {
	auto node_processor = std::make_unique<blueprint::NodeProcessor>();
		
	for (auto& node : nodes) {
		switch (node->type) {
			case NodeType::KeyPress:
				node_processor->spawn_node<blueprint::KeyPressNode>(std::nullopt, node->position());
				break;
			case NodeType::KeyRelease:
				node_processor->spawn_node<blueprint::KeyReleaseNode>(std::nullopt, node->position());
				break;
			case NodeType::String:
				node_processor->spawn_node<blueprint::StringNode>(std::nullopt, node->position());
				break;
			case NodeType::Print:
				node_processor->spawn_node<blueprint::PrintNode>(std::nullopt, node->position());
				break;
		}
	}
	
	for (auto& link : links) {
		
	}
	
	actor.add_component<BlueprintComponent>(std::move(node_processor));
}

void blueprint::NodeProcessor::deserialize(BlueprintCanvas& canvas, Actor& actor) {
	next_id = 1;
	auto& blueprint_component = actor.get_component<BlueprintComponent>();
	
	auto& node_processor = blueprint_component.node_processor();
	
	for (auto& node : node_processor.nodes) {
		switch (node->type) {
			case NodeType::KeyPress:
				spawn_node<blueprint::KeyPressNode>(canvas, node->position());
				break;
			case NodeType::KeyRelease:
				spawn_node<blueprint::KeyReleaseNode>(canvas, node->position());
				break;
			case NodeType::String:
				spawn_node<blueprint::StringNode>(canvas, node->position());
				break;
			case NodeType::Print:
				spawn_node<blueprint::PrintNode>(canvas, node->position());
				break;
		}
	}
	
	for (auto& link : links) {
		
	}

}


void blueprint::NodeProcessor::clear() {
	nodes.clear();
	links.clear();
}
