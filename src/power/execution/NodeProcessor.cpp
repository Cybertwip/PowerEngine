#include "NodeProcessor.hpp"

#include "BlueprintCanvas.hpp"
#include "Node.hpp"
#include "StringNode.hpp"
#include "PrintNode.hpp"

namespace blueprint {
NodeProcessor::NodeProcessor(BlueprintCanvas& canvas)
: mCanvas(canvas) {
	
}

int NodeProcessor::get_next_id() {
	return next_id++;
}

void NodeProcessor::build_node(Node& node){
	node.build();
}

void NodeProcessor::spawn_string_node(const nanogui::Vector2i& position) {
	auto node = std::make_unique<StringNode>(mCanvas, "String",  nanogui::Vector2i(196, 64), get_next_id(), get_next_id());
	node->set_position(position);
	build_node(*node);
	nodes.push_back(std::move(node));
	mCanvas.add_node(nodes.back().get());
}

void NodeProcessor::spawn_print_string_node(const nanogui::Vector2i& position) {
	auto node = std::make_unique<PrintNode>(mCanvas, "Print",  nanogui::Vector2i(128, 64), get_next_id(), get_next_id(), get_next_id(), get_next_id());
	node->set_position(position);
	build_node(*node);
	nodes.push_back(std::move(node));
	mCanvas.add_node(nodes.back().get());
}

//
//	Node& spawn_input_action_node(const std::string& key_string, int key_code) {
//		auto label = "Input Action " + key_string;
//		auto node = std::make_unique<Node>(get_next_id(), label.c_str(), nanogui::Color(255, 128, 128, 255));
//		node->root_node = true;
//		node->outputs.emplace_back(get_next_id(), node->id, "Pressed", PinType::Flow);
//		node->outputs.emplace_back(get_next_id(), node->id, "Released", PinType::Flow);
//
//		node->evaluate = [node = node.get(), key_code]() {
//			if (key_code != -1) {
//				if (glfwGetKey(glfwGetCurrentContext(), key_code) == GLFW_PRESS) {
//					node->outputs[0].can_flow = true;
//				} else if (glfwGetKey(glfwGetCurrentContext(), key_code) == GLFW_RELEASE) {
//					if (node->outputs[0].can_flow) {
//						node->outputs[0].can_flow = false;
//						node->outputs[1].can_flow = true;
//					} else {
//						node->outputs[1].can_flow = false;
//					}
//				}
//			} else {
//				node->outputs[0].can_flow = false;
//				node->outputs[1].can_flow = false;
//			}
//		};
//
//		build_node(node.get());
//		nodes.push_back(std::move(node));
//		return *nodes.back().get();
//	}

void NodeProcessor::evaluate(Node& node) {
	if (node.evaluate) {
		node.evaluate();
	}
}
}
