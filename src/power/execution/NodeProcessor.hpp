#pragma once

#include <memory>
#include <vector>

namespace blueprint {
class BlueprintCanvas;
class Node;
class Pin;
class Link;

class NodeProcessor {
public:
	NodeProcessor();
	
	int get_next_id();
	
	void build_node(BlueprintNode& node);

	Link* create_link(BlueprintCanvas& parent, Pin& output, Pin& input);

	BlueprintNode* spawn_string_node(BlueprintCanvas& parent, const nanogui::Vector2i& position);
	BlueprintNode* spawn_print_string_node(BlueprintCanvas& parent, const nanogui::Vector2i& position);
	BlueprintNode* spawn_key_press_node(BlueprintCanvas& parent, const nanogui::Vector2i& position);

	void evaluate(BlueprintNode& node);
private:
	int next_id = 1;
	std::vector<std::unique_ptr<BlueprintNode>> nodes;
	std::vector<std::unique_ptr<Link>> links;
};

}
