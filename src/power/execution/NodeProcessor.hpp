#pragma once

#include "BlueprintNode.hpp"

#include <memory>
#include <vector>

class Actor;

class BlueprintCanvas;
class BlueprintSerializer;

class NodeProcessor {
public:
	NodeProcessor();
	
	long long get_next_id();
	
	void build_node(CoreNode& node);

	void evaluate();
	
	void serialize(BlueprintCanvas& canvas, Actor& actor);
	void deserialize(BlueprintCanvas& canvas, Actor& actor);
	void clear();
	
	template<typename T>
	T& spawn_node(long long id) {
		auto node = std::make_unique<T>(id);
		build_node(*node);
		T& node_ref = *node;
		nodes.push_back(std::move(node));
		return node_ref;
	}
	
	void create_link(long long id, CorePin& output, CorePin& input){
		auto link = std::make_unique<Link>(id, output, input);

		output.links.push_back(link.get());
		input.links.push_back(link.get());

		links.push_back(std::move(link));
	}

	void create_link(BlueprintCanvas& canvas, long long id, VisualPin& output, VisualPin& input){
		auto link = std::make_unique<Link>(id, output.core_pin(), input.core_pin());
		links.push_back(std::move(link));
		
		output.core_pin().links.push_back(links.back().get());
		input.core_pin().links.push_back(links.back().get());
		
		canvas.add_link(output, input);
	}
	
	CoreNode& get_node(long long id);
	
	void break_links(CoreNode& node);
	
public:
	const std::vector<std::unique_ptr<CoreNode>>& get_nodes() const { return nodes; }
	const std::vector<std::unique_ptr<Link>>& get_links() const { return links; }


private:
	void set_next_id(long long id);
	
	long long next_id;
	std::vector<std::unique_ptr<Link>> links;
	std::vector<std::unique_ptr<CoreNode>> nodes;
	
	friend class BlueprintSerializer;

};
