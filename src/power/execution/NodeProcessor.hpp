#pragma once

#include <memory>
#include <vector>

class Actor;

namespace blueprint {
class BlueprintCanvas;
class Node;
class Pin;
class Link;

class NodeProcessor {
public:
	NodeProcessor();
	
	long long get_next_id();
	
	void build_node(BlueprintNode& node);

	Link* create_link(BlueprintCanvas& parent, Pin& output, Pin& input);

	BlueprintNode* spawn_string_node(BlueprintCanvas& parent, const nanogui::Vector2i& position);
	BlueprintNode* spawn_print_string_node(BlueprintCanvas& parent, const nanogui::Vector2i& position);
	BlueprintNode* spawn_key_press_node(BlueprintCanvas& parent, const nanogui::Vector2i& position);
	BlueprintNode* spawn_key_release_node(BlueprintCanvas& parent, const nanogui::Vector2i& position);

	void evaluate();
	
	void serialize(Actor& actor);
	void deserialize(BlueprintCanvas& canvas, Actor& actor);
	void clear();
	
private:
	template<typename T>
	blueprint::BlueprintNode* spawn_node(std::optional<std::reference_wrapper<blueprint::BlueprintCanvas>> parent, const nanogui::Vector2i& position) {
		auto node = std::make_unique<T>(parent, "String",  nanogui::Vector2i(196, 64), [this](){
			return get_next_id();
		});
		node->set_position(position);
		build_node(*node);
		nodes.push_back(std::move(node));
		return nodes.back().get();
	}
	
	
	long long next_id = 1;
	std::vector<std::unique_ptr<BlueprintNode>> nodes;
	std::vector<std::unique_ptr<Link>> links;
};

}
