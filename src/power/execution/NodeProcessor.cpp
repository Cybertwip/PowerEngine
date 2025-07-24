#include "execution/NodeProcessor.hpp"
#include "execution/BlueprintCanvas.hpp"
#include "actors/Actor.hpp"
#include "components/BlueprintComponent.hpp"
#include "serialization/UUID.hpp"

// --- Include all concrete node types we can spawn ---
#include "KeyPressNode.hpp"
#include "KeyReleaseNode.hpp"
#include "ReflectedNode.hpp"


NodeProcessor::NodeProcessor() {
	// 1. Register hardcoded, non-reflected nodes (like events).
	m_creators["KeyPress"] = [](UUID id) {
		return std::make_unique<KeyPressCoreNode>(id);
	};
	m_creators["KeyRelease"] = [](UUID id) {
		return std::make_unique<KeyReleaseCoreNode>(id);
	};
	
	// 2. Discover all reflected C++ types that should be available as nodes.
	//    This assumes the reflection registry is now responsible for creating
	//    and storing the node creator functions.
	const auto all_reflected_types = power::reflection::ReflectionRegistry::get_all_types();
	for (const auto& type_info : all_reflected_types) {
		if (type_info.is_valid() && type_info.has_node_creator()) {
			m_creators[type_info.get_name()] = type_info.get_node_creator();
		}
	}
}

long long NodeProcessor::get_next_id() {
	
	return UUIDGenerator::generate();
	
}



void NodeProcessor::build_node(CoreNode& node){
	
	node.build();
	
}

void NodeProcessor::break_links(CoreNode& node) {
	
	
	std::vector<Link*> links_to_remove;
	
	
	for (auto& link : links) {
		
		const auto& start_pin = link->get_start();
		
		const auto& end_pin = link->get_end();
		
		
		if (&start_pin.node == &node || &end_pin.node == &node) {
			
			links_to_remove.push_back(link.get());
			
		}
		
	}
	
	
	links.erase(
				
				std::remove_if(links.begin(), links.end(),
							   
							   [&links_to_remove](const std::unique_ptr<Link>& link) {
								   
								   return std::find(links_to_remove.begin(), links_to_remove.end(), link.get()) != links_to_remove.end();
								   
							   }
							   
							   ),
				
				links.end()
				
				);
	
}

std::unique_ptr<CoreNode> NodeProcessor::create_node(const std::string& type_name, UUID id) {
	if (m_creators.count(type_name)) {
		// Call the stored creator function for the requested type.
		return m_creators.at(type_name)(id);
	}
	std::cerr << "ERROR: Attempted to create unknown node type: " << type_name << std::endl;
	return nullptr;
}

CoreNode& NodeProcessor::add_node(std::unique_ptr<CoreNode> node) {
	if (!node) {
		throw std::runtime_error("Attempted to add a null node to NodeProcessor.");
	}
	CoreNode& ref = *node;
	nodes.push_back(std::move(node));
	return ref;
}

void NodeProcessor::serialize(BlueprintCanvas& canvas, Actor& actor) {
	auto runtime_processor = std::make_unique<NodeProcessor>();
	
	if (actor.find_component<BlueprintComponent>()) {
		actor.remove_component<BlueprintComponent>();
	}
	
	// 1. Clone all nodes from the canvas processor to the runtime processor.
	for (const auto& canvas_node : this->nodes) {
		std::unique_ptr<CoreNode> new_node = runtime_processor->create_node(canvas_node->reflected_type_name, canvas_node->id);
		if (new_node) {
			new_node->set_position(canvas_node->position);
			
			// If it's a reflected node, copy its internal C++ object's state.
			if(auto* src = dynamic_cast<ReflectedCoreNode*>(canvas_node.get())) {
				if(auto* dst = dynamic_cast<ReflectedCoreNode*>(new_node.get())) {
					dst->get_instance() = src->get_instance();
				}
			}
			// For event nodes, we might need to copy specific state.
			else if (auto* src = dynamic_cast<KeyPressCoreNode*>(canvas_node.get())) {
				if(auto* dst = dynamic_cast<KeyPressCoreNode*>(new_node.get())) {
					dst->set_window(canvas.screen().glfw_window());
				}
			}
			runtime_processor->add_node(std::move(new_node));
		}
	}
	
	// 2. Clone all links.
	for (const auto& link : this->links) {
		auto& start_pin = link->get_start();
		auto& end_pin = link->get_end();
		
		auto* new_start_node = runtime_processor->find_node(start_pin.node.id);
		auto* new_end_node = runtime_processor->find_node(end_pin.node.id);
		
		if (new_start_node && new_end_node) {
			runtime_processor->create_link(link->get_id(), new_start_node->get_pin(start_pin.id), new_end_node->get_pin(end_pin.id));
		}
	}
	
	actor.add_component<BlueprintComponent>(std::move(runtime_processor));
}

void NodeProcessor::deserialize(BlueprintCanvas& canvas, Actor& actor) {
	canvas.clear();
	this->clear();
	
	if (!actor.find_component<BlueprintComponent>()) return;
	
	auto& bp_comp = actor.get_component<BlueprintComponent>();

	
	auto& source_processor = bp_comp.node_processor();
	
	// 1. Create all nodes in this processor from the source processor's data.
	for (const auto& source_node : source_processor.get_nodes()) {
		std::unique_ptr<CoreNode> new_node = this->create_node(source_node->reflected_type_name, source_node->id);
		if (new_node) {
			new_node->set_position(source_node->position);
			
			// Copy instance data for reflected nodes.
			if(auto* src = dynamic_cast<ReflectedCoreNode*>(source_node.get())) {
				if(auto* dst = dynamic_cast<ReflectedCoreNode*>(new_node.get())) {
					dst->get_instance() = src->get_instance();
				}
			}
			
			// Spawn the corresponding VISUAL node on the canvas.
			if (auto* reflected_core = dynamic_cast<ReflectedCoreNode*>(new_node.get())) {
				canvas.spawn_node<ReflectedVisualNode>(new_node->position, *reflected_core);
			} else if (auto* keypress_core = dynamic_cast<KeyPressCoreNode*>(new_node.get())) {
				canvas.spawn_node<KeyPressVisualNode>(new_node->position, *keypress_core);
			} // ... add else-if for other non-reflected visual node types.
			
			this->add_node(std::move(new_node));
		}
	}
	
	// 2. Recreate all links, both logically and visually.
	for (const auto& link : source_processor.get_links()) {
		auto& start_pin = link->get_start();
		auto& end_pin = link->get_end();
		
		auto* new_start_node = this->find_node(start_pin.node.id);
		auto* new_end_node = this->find_node(end_pin.node.id);
		
		if (new_start_node && new_end_node) {
			auto& new_start_pin = new_start_node->get_pin(start_pin.id);
			auto& new_end_pin = new_end_node->get_pin(end_pin.id);
			
			this->create_link(link->get_id(), new_start_pin, new_end_pin);
			canvas.link(new_start_pin, new_end_pin); // Tell canvas to draw the link
		}
	}
	canvas.perform_layout(canvas.screen().nvg_context());
}


// --- Unchanged Methods ---

void NodeProcessor::create_link(UUID id, CorePin& output, CorePin& input){
	auto link = std::make_unique<Link>(id, output, input);
	output.links.push_back(link.get());
	input.links.push_back(link.get());
	links.push_back(std::move(link));
}

void NodeProcessor::create_link(BlueprintCanvas& canvas, UUID id, VisualPin& output, VisualPin& input){
	auto link = std::make_unique<Link>(id, output.core_pin(), input.core_pin());
	output.core_pin().links.push_back(link.get());
	input.core_pin().links.push_back(link.get());
	links.push_back(std::move(link));
	canvas.add_link(output, input);
}

CoreNode* NodeProcessor::find_node(UUID id) {
	auto it = std::find_if(nodes.begin(), nodes.end(),
						   [id](const std::unique_ptr<CoreNode>& node) {
		return node->id == id;
	});
	return (it != nodes.end()) ? it->get() : nullptr;
}

void NodeProcessor::clear() {
	nodes.clear();
	links.clear();
}
