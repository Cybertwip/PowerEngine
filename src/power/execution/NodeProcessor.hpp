#pragma once

#include <memory>
#include <vector>

class Actor;

class BlueprintCanvas;
class BlueprintNode;
class Pin;
class Link;

class NodeProcessor {
public:
	NodeProcessor();
	
	long long get_next_id();
	
	void build_node(BlueprintNode& node);

	void evaluate();
	
	void serialize(BlueprintCanvas& canvas, Actor& actor);
	void deserialize(BlueprintCanvas& canvas, Actor& actor);
	void clear();
	
	template<typename T>
	BlueprintNode* spawn_node(BlueprintCanvas& parent, const nanogui::Vector2i& position) {
		auto node = std::make_unique<T>(parent, nanogui::Vector2i(196, 64), [this](){
			return get_next_id();
		});
		node->set_position(position);
		build_node(*node);
		nodes.push_back(std::move(node));
		
		return nodes.back().get();
	}
	
	Link* create_link(BlueprintCanvas& parent, long long id, Pin& output, Pin& input){
		auto link = std::make_unique<Link>(parent, id, output, input);
		links.push_back(std::move(link));
		parent.add_link(links.back().get());
		return links.back().get();
	}
	
	Pin* find_pin(long long id);
	BlueprintNode* find_node(long long id);
	
	void break_links(BlueprintNode* node);

private:
	long long next_id = 1;
	std::vector<std::unique_ptr<BlueprintNode>> nodes;
	std::vector<std::unique_ptr<Link>> links;
};
