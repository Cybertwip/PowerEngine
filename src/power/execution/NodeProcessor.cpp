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

NodeProcessor::NodeProcessor() : next_id(1) {
	
}

long long NodeProcessor::get_next_id() {
	return next_id++;
}

void NodeProcessor::build_node(CoreNode& node){
	node.build();
}

void NodeProcessor::evaluate() {
	std::unordered_set<CoreNode*> evaluated_nodes;
	
	for (auto& link : links)
	{
		auto& out_pin = link->get_start();
		auto& in_pin = link->get_end();
		
		if(evaluated_nodes.find(&out_pin.node) == evaluated_nodes.end()){
			if(out_pin.node.evaluate()){
				evaluated_nodes.insert(&out_pin.node);
			}
		}
		
		if(evaluated_nodes.find(&in_pin.node) == evaluated_nodes.end()){
			if(in_pin.node.evaluate()){
				evaluated_nodes.insert(&in_pin.node);
			}
		}
		
		
		if (in_pin.type == PinType::Flow) {
			bool can_flow = false; // Initialize to false
			
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
	auto node_processor = std::make_unique<NodeProcessor>();
	
	if(actor.find_component<BlueprintComponent>()) {
		actor.remove_component<BlueprintComponent>();
	}
	
	if (!nodes.empty()) {
		for (auto& node : nodes) {
			switch (node->type) {
				case NodeType::KeyPress: {
					auto& spawn = node_processor->spawn_node<KeyPressCoreNode>(node->id);
					spawn.set_window(canvas.screen().glfw_window());
					spawn.set_position(node->position);
				}
					break;
				case NodeType::KeyRelease: {
					auto& spawn = node_processor->spawn_node<KeyReleaseCoreNode>(node->id);
					spawn.set_window(canvas.screen().glfw_window());
					spawn.set_position(node->position);
				}
					break;
				case NodeType::String: {
					auto& spawn = node_processor->spawn_node<StringCoreNode>(node->id);
					spawn.set_position(node->position);
				}
					break;
					
				case NodeType::Print: {
					auto& spawn = node_processor->spawn_node<PrintCoreNode>(node->id);
					spawn.set_position(node->position);
				}
					break;
			}
			
			auto& source_node = *node;
			// Safely find the newly created node
			if (auto* target_node = node_processor->find_node(node->id)) {
				const auto& source_inputs = source_node.get_inputs();
				const auto& target_inputs = target_node->get_inputs();
				
				for (const auto& source_pin : source_inputs) {
					for (const auto& target_pin : target_inputs) {
						if (target_pin) {
							target_pin->set_data(source_pin->get_data());
						}
					}
				}
				
				const auto& source_outputs = source_node.get_outputs();
				const auto& target_outputs = target_node->get_outputs();
				
				for (const auto& source_pin : source_outputs) {
					for (const auto& target_pin : target_outputs) {
						if (target_pin) {
							target_pin->set_data(source_pin->get_data());
						}
					}
				}
			}
		}
		
		for (auto& link : links) {
			auto& start_pin = link->get_start();
			auto& end_pin = link->get_end();
			
			// Safely find the source and destination nodes before creating a link
			auto* target_source_node = node_processor->find_node(start_pin.node.id);
			auto* target_destination_node = node_processor->find_node(end_pin.node.id);
			
			if (target_source_node && target_destination_node) {
				auto& target_start_pin = target_source_node->get_pin(start_pin.id);
				auto& target_end_pin = target_destination_node->get_pin(end_pin.id);
				node_processor->create_link(link->get_id(), target_start_pin, target_end_pin);
			}
		}
		
		for (auto& node : nodes) {
			// Safely find the target node before attempting a dynamic_cast
			if (auto* target_node = node_processor->find_node(node->id)) {
				auto* that_node = dynamic_cast<DataCoreNode*>(target_node);
				auto* this_node = dynamic_cast<DataCoreNode*>(node.get());
				
				if (that_node && this_node) {
					// Check if any of the original node's pins are connected in the current (original) graph.
					// This prevents transferring data for disconnected data nodes.
					bool is_connected = false;
					const auto check_pins = [this, &is_connected](const auto& pins) {
						for (const auto& pin : pins) {
							if (pin && this->is_pin_connected(*pin)) {
								is_connected = true;
							}
						}
					};
					
					check_pins(this_node->get_inputs());
					if (!is_connected) {
						check_pins(this_node->get_outputs());
					}
					
					// Only transfer data if the node is part of a connection.
					if (is_connected) {
						that_node->set_data(this_node->get_data());
					}
				}
			}
		}
		
	}
	
	actor.add_component<BlueprintComponent>(std::move(node_processor));
}

void NodeProcessor::deserialize(BlueprintCanvas& canvas, Actor& actor) {
	
	canvas.clear(); // must clean after deallocation
	clear();
	
	if (actor.find_component<BlueprintComponent>()) {
		auto& blueprint_component = actor.get_component<BlueprintComponent>();
		
		auto& node_processor = blueprint_component.node_processor();
		
		for (auto& node : node_processor.nodes) {
			switch (node->type) {
				case NodeType::KeyPress:
					canvas.spawn_node<KeyPressVisualNode>(node->position, spawn_node<KeyPressCoreNode>(node->id));
					break;
				case NodeType::KeyRelease:
					canvas.spawn_node<KeyReleaseVisualNode>(node->position, spawn_node<KeyReleaseCoreNode>(node->id));
					break;
				case NodeType::String:
					canvas.spawn_node<StringVisualNode>(node->position, spawn_node<StringCoreNode>(node->id));
					break;
					
				case NodeType::Print:
					canvas.spawn_node<PrintVisualNode>(node->position, spawn_node<PrintCoreNode>(node->id));
					break;
			}
			
			auto& source_node = *node;
			// Safely find the newly created node
			if (auto* target_node = find_node(node->id)) {
				const auto& source_inputs = source_node.get_inputs();
				const auto& target_inputs = target_node->get_inputs();
				
				for (const auto& source_pin : source_inputs) {
					for (const auto& target_pin : target_inputs) {
						if (target_pin) {
							target_pin->set_data(source_pin->get_data());
						}
					}
				}
				
				const auto& source_outputs = source_node.get_outputs();
				const auto& target_outputs = target_node->get_outputs();
				
				for (const auto& source_pin : source_outputs) {
					for (const auto& target_pin : target_outputs) {
						if (target_pin) {
							target_pin->set_data(source_pin->get_data());
						}
					}
				}
			}
		}
		
		for (auto& node : node_processor.nodes) {
			// Safely find the target node before attempting a dynamic_cast
			if(auto* target_node = find_node(node->id)) {
				auto* this_node = dynamic_cast<DataCoreNode*>(target_node);
				auto* that_node = dynamic_cast<DataCoreNode*>(node.get());
				
				if (this_node && that_node) {
					this_node->set_data(that_node->get_data());
				}
			}
		}
		
		for (auto& link : node_processor.links) {
			auto& start_pin = link->get_start();
			auto& end_pin = link->get_end();
			
			// Safely find the source and destination nodes before creating a link
			auto* target_source_node = find_node(start_pin.node.id);
			auto* target_destination_node = find_node(end_pin.node.id);
			
			if (target_source_node && target_destination_node) {
				auto& target_start_pin = target_source_node->get_pin(start_pin.id);
				auto& target_end_pin = target_destination_node->get_pin(end_pin.id);
				
				create_link(link->get_id(), target_start_pin, target_end_pin);
				canvas.link(target_start_pin, target_end_pin);
			}
		}
		
		canvas.perform_layout(canvas.screen().nvg_context());
	}
}

CoreNode& NodeProcessor::get_node(long long id) {
	auto node_it = std::find_if(nodes.begin(), nodes.end(), [id](auto& node) {
		return node->id == id;
	});
	
	// WARNING: This will crash if the node is not found. Consider using find_node() instead.
	if (node_it == nodes.end()) {
		// This is a fallback to prevent a crash, but indicates a logic error.
		// A more robust solution would be to throw an exception or handle the error gracefully.
		throw std::runtime_error("Node with ID " + std::to_string(id) + " not found.");
	}
	return *node_it->get();
}

/**
 * @brief This is the implementation for the new safe find_node function.
 */
CoreNode* NodeProcessor::find_node(long long id) {
	// Use std::find_if to locate the node with the matching ID.
	auto node_it = std::find_if(nodes.begin(), nodes.end(), [id](const std::unique_ptr<CoreNode>& node) {
		return node->id == id;
	});
	
	// If the iterator is not at the end, the node was found.
	if (node_it != nodes.end()) {
		// Return a raw pointer to the node.
		return node_it->get();
	}
	
	// If the node was not found, return nullptr.
	return nullptr;
}

/**
 * @brief Checks if a given pin is connected to any link.
 * @param pin The pin to check.
 * @return True if the pin is part of at least one link, false otherwise.
 */
bool NodeProcessor::is_pin_connected(const CorePin& pin) const {
	// Iterate through all links to see if the given pin is a start or end point.
	return std::any_of(links.begin(), links.end(), [&pin](const std::unique_ptr<Link>& link) {
		return &link->get_start() == &pin || &link->get_end() == &pin;
	});
}


void NodeProcessor::break_links(CoreNode& node) {
	
	// Create a vector to store links that need to be removed
	std::vector<Link*> links_to_remove;
	
	// Find all links connected to the node's input or output pins
	for (auto& link : links) {
		const auto& start_pin = link->get_start();
		const auto& end_pin = link->get_end();
		
		// Check if either end of the link connects to the node
		if (&start_pin.node == &node || &end_pin.node == &node) {
			links_to_remove.push_back(link.get());
		}
	}
	
	// Remove the identified links
	links.erase(
				std::remove_if(links.begin(), links.end(),
							   [&links_to_remove](const std::unique_ptr<Link>& link) {
								   return std::find(links_to_remove.begin(), links_to_remove.end(), link.get()) != links_to_remove.end();
							   }
							   ),
				links.end()
				);
}

void NodeProcessor::clear() {
	nodes.clear();
	links.clear();
	next_id = 1;
}
