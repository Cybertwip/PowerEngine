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
	
	blueprint::NodeProcessor& node_processor() {
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

void blueprint::NodeProcessor::serialize(BlueprintCanvas& canvas, Actor& actor) {
	if (actor.find_component<BlueprintComponent>()) {
		actor.remove_component<BlueprintComponent>();
	}
	
	if (!nodes.empty()) {
		auto node_processor = std::make_unique<blueprint::NodeProcessor>();
		
		for (auto& node : nodes) {
			switch (node->type) {
				case NodeType::KeyPress:
					node_processor->spawn_node<blueprint::KeyPressNode>(canvas, node->position());
					break;
				case NodeType::KeyRelease:
					node_processor->spawn_node<blueprint::KeyReleaseNode>(canvas, node->position());
					break;
				case NodeType::String:
					node_processor->spawn_node<blueprint::StringNode>(canvas, node->position());
					break;
				case NodeType::Print:
					node_processor->spawn_node<blueprint::PrintNode>(canvas, node->position());
					break;
			}
		}
		
		for (auto& link : links) {
			auto* start_pin = node_processor->find_pin(link->get_start().id);
			auto* end_pin = node_processor->find_pin(link->get_end().id);
			
			assert(start_pin && end_pin);
			
			if (link->get_start().data.has_value()) {
				start_pin->data = link->get_start().data;
			}

			if (link->get_end().data.has_value()) {
				end_pin->data = link->get_end().data;
			}

			node_processor->create_link(canvas, link->id, *start_pin, *end_pin);
		}
		
		actor.add_component<BlueprintComponent>(std::move(node_processor));
	}
	
}

void blueprint::NodeProcessor::deserialize(BlueprintCanvas& canvas, Actor& actor) {
	next_id = 1;
	
	if (actor.find_component<BlueprintComponent>()) {
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
		
		for (auto& link : node_processor.links) {
			auto* start_pin = find_pin(link->get_start().id);
			auto* end_pin = find_pin(link->get_end().id);
			
			assert(start_pin && end_pin);
			
			if (link->get_start().data.has_value()) {
				start_pin->data = link->get_start().data;
			}
			
			if (link->get_end().data.has_value()) {
				end_pin->data = link->get_end().data;
			}
			
			create_link(canvas, link->id, *start_pin, *end_pin);
		}
		
		canvas.perform_layout(canvas.screen().nvg_context());
	}
}

blueprint::Pin* blueprint::NodeProcessor::find_pin(long long id) {
	
	for (auto& node : nodes) {
		const auto& node_inputs = node->get_inputs();
		const auto& node_outputs = node->get_outputs();
		
		auto start_pin_it = std::find_if(node_outputs.begin(), node_outputs.end(), [id](auto& pin) {
			return pin->id == id;
		});
		
		if (start_pin_it != node_outputs.end()) {
			return start_pin_it->get();
		}
		
		auto end_pin_it = std::find_if(node_inputs.begin(), node_inputs.end(), [id](auto& pin) {
			return pin->id == id;
		});
		
		if (end_pin_it != node_inputs.end()) {
			return end_pin_it->get();
		}
	}
	
	return nullptr;
}

void blueprint::NodeProcessor::clear() {
	nodes.clear();
	links.clear();
	next_id = 1;
}
