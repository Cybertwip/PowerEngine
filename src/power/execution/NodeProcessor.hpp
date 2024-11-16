#pragma once

#include <nanogui/vector.h>

#include <memory>
#include <vector>

namespace blueprint {
class BlueprintCanvas;
class Node;
class Pin;
struct Link;

class NodeProcessor {
public:
	NodeProcessor(BlueprintCanvas& canvas);
	
	int get_next_id();
	
	void build_node(Node& node);
	
	void spawn_string_node(const nanogui::Vector2i& position);
	void spawn_print_string_node(const nanogui::Vector2i& position);
	
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
	
	void evaluate(Node& node);
private:
	BlueprintCanvas& mCanvas;
	
	int next_id = 1;
	std::vector<std::unique_ptr<Node>> nodes;
	std::vector<std::unique_ptr<Link>> links;
};

}
