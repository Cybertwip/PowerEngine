#include "NodeProcessor.hpp"

#include "Canvas.hpp"

#include "actors/Actor.hpp"

#include "BlueprintNode.hpp"
#include "BlueprintCanvas.hpp"
#include "KeyPressNode.hpp"
#include "KeyReleaseNode.hpp"
#include "PrintNode.hpp"
#include "StringNode.hpp"

#include "components/BlueprintComponent.hpp"

#include <unordered_set>

NodeProcessor::NodeProcessor() {
	
}

long long NodeProcessor::get_next_id() {
	return next_id++;
}

void NodeProcessor::build_node(BlueprintNode& node){
	node.build();
}

void NodeProcessor::evaluate() {

	std::unordered_set<BlueprintNode*> evaluated_nodes;
	
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
		
		in_pin.set_data(out_pin.get_data());
	}
}

void NodeProcessor::serialize(BlueprintCanvas& canvas, Actor& actor) {
	if (actor.find_component<BlueprintComponent>()) {
		actor.remove_component<BlueprintComponent>();
	}
	
	if (!nodes.empty()) {
		auto node_processor = std::make_unique<NodeProcessor>();
		
		for (auto& node : nodes) {
			switch (node->type) {
				case NodeType::KeyPress:
					node_processor->spawn_node<KeyPressNode>(canvas, node->position());
					break;
				case NodeType::KeyRelease:
					node_processor->spawn_node<KeyReleaseNode>(canvas, node->position());
					break;
				case NodeType::String:
					node_processor->spawn_node<StringNode>(canvas, node->position());
					break;
				case NodeType::Print:
					node_processor->spawn_node<PrintNode>(canvas, node->position());
					break;
			}
		}
		
		for (auto& node : nodes) {
			auto* that_node = dynamic_cast<BlueprintDataNode*>(node_processor->find_node(node->id));

			auto* this_node = dynamic_cast<BlueprintDataNode*>(node.get());
			
			if (that_node && this_node) {
				that_node->set_data(this_node->get_data());
			}
		}
		
		for (auto& link : links) {
			auto* start_pin = node_processor->find_pin(link->get_start().id);
			auto* end_pin = node_processor->find_pin(link->get_end().id);
			
			assert(start_pin && end_pin);
			
			if (link->get_start().get_data().has_value()) {
				start_pin->set_data(link->get_start().get_data());
			}

			if (link->get_end().get_data().has_value()) {
				end_pin->set_data(link->get_end().get_data());
			}

			node_processor->create_link(canvas, link->get_id(), *start_pin, *end_pin);
		}
		
		actor.add_component<BlueprintComponent>(std::move(node_processor));
	}
	
}

void NodeProcessor::deserialize(BlueprintCanvas& canvas, Actor& actor) {
	next_id = 1;
	
	if (actor.find_component<BlueprintComponent>()) {
		auto& blueprint_component = actor.get_component<BlueprintComponent>();
		
		auto& node_processor = blueprint_component.node_processor();
		
		for (auto& node : node_processor.nodes) {
			switch (node->type) {
				case NodeType::KeyPress:
					spawn_node<KeyPressNode>(canvas, node->position());
					break;
				case NodeType::KeyRelease:
					spawn_node<KeyReleaseNode>(canvas, node->position());
					break;
				case NodeType::String:
					spawn_node<StringNode>(canvas, node->position());
					break;
				case NodeType::Print:
					spawn_node<PrintNode>(canvas, node->position());
					break;
			}
		}
		
		for (auto& node : node_processor.nodes) {
			auto* this_node = dynamic_cast<BlueprintDataNode*>(find_node(node->id));
			
			auto* that_node = dynamic_cast<BlueprintDataNode*>(node.get());
			
			if (this_node && that_node) {
				this_node->set_data(that_node->get_data());
			}

		}
		
		for (auto& link : node_processor.links) {
			auto* start_pin = find_pin(link->get_start().id);
			auto* end_pin = find_pin(link->get_end().id);
			
			assert(start_pin && end_pin);
			
			if (link->get_start().get_data().has_value()) {
				start_pin->set_data(link->get_start().get_data());
			}
			
			if (link->get_end().get_data().has_value()) {
				end_pin->set_data(link->get_end().get_data());
			}
			
			create_link(canvas, link->get_id(), *start_pin, *end_pin);
		}
		
		canvas.perform_layout(canvas.screen().nvg_context());
	}
}

Pin* NodeProcessor::find_pin(long long id) {
	
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


BlueprintNode* NodeProcessor::find_node(long long id) {

	auto node_it = std::find_if(nodes.begin(), nodes.end(), [id](auto& node) {
		return node->id == id;
	});

	if (node_it != nodes.end()) {
		return node_it->get();
	}

	return nullptr;
}


void NodeProcessor::clear() {
	nodes.clear();
	links.clear();
	next_id = 1;
}
