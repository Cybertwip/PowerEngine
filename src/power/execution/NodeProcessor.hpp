#pragma once

#include "BlueprintNode.hpp"
#include "reflection/PowerReflection.hpp" // For type registration

#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <map>
#include <algorithm>
#include <any>

// Forward declarations
class Actor;
class BlueprintCanvas;

/**
 * @class NodeProcessor
 * @brief Manages the lifecycle and logic of all CoreNodes in a blueprint graph.
 *
 * This class is responsible for creating, storing, connecting (linking), and
 * serializing/deserializing the logical part of the blueprint. It uses a
 * creator pattern, populated by the reflection system, to dynamically
 * instantiate any registered node type.
 */
class NodeProcessor {
public:
	/**
	 * @brief Constructs a NodeProcessor and discovers all available node types from the reflection system.
	 */
	NodeProcessor();
	
	// --- Core Public API ---
	
	/**
	 * @brief Creates a new node instance by its registered type name.
	 * @param type_name The name the node was registered with (e.g., "Transform", "KeyPress").
	 * @param id The UUID for the new node.
	 * @return A unique_ptr to the newly created CoreNode, or nullptr if the type is unknown.
	 */
	std::unique_ptr<CoreNode> create_node(const std::string& type_name, UUID id);
	
	/**
	 * @brief Adds a fully created node to the processor's managed list.
	 * @param node A unique_ptr to the CoreNode to add.
	 * @return A reference to the added node.
	 */
	CoreNode& add_node(std::unique_ptr<CoreNode> node);
	
	/**
	 * @brief Creates a runtime copy of the current node graph and attaches it to an Actor.
	 * This is used for saving or preparing the blueprint for execution.
	 * @param canvas The UI canvas (needed for context like the window handle).
	 * @param actor The actor to attach the resulting BlueprintComponent to.
	 */
	void serialize(BlueprintCanvas& canvas, Actor& actor);
	
	/**
	 * @brief Clears the current node graph and rebuilds it from an Actor's BlueprintComponent.
	 * This is used for loading a blueprint into the editor.
	 * @param canvas The UI canvas to spawn the visual nodes on.
	 * @param actor The actor containing the blueprint data to load.
	 */
	void deserialize(BlueprintCanvas& canvas, Actor& actor);
	
	/**
	 * @brief Clears all nodes and links from the processor.
	 */
	void clear();
	
	// --- Restored Helper Methods ---
	
	long long get_next_id();
	void build_node(CoreNode& node);
	void break_links(CoreNode& node);
	
	template<typename T>
	T& spawn_node(UUID id) {
		auto node = std::make_unique<T>(id);
		build_node(*node);
		T& node_ref = *node;
		nodes.push_back(std::move(node));
		return node_ref;
	}
	
	void create_link(UUID id, CorePin& output, CorePin& input);
	void create_link(BlueprintCanvas& canvas, UUID id, VisualPin& output, VisualPin& input);
	
	CoreNode& get_node(UUID id);
	CoreNode* find_node(UUID id);
	
	const std::vector<std::unique_ptr<CoreNode>>& get_nodes() const { return nodes; }
	const std::vector<std::unique_ptr<Link>>& get_links() const { return links; }
	
private:
	// A function that can create a CoreNode.
	using CreatorFunc = std::function<std::unique_ptr<CoreNode>(UUID)>;
	
private:
	// Maps a string name to a function that can create the corresponding node.
	std::map<std::string, CreatorFunc> m_creators;
	
	// The collections of all logical nodes and links in the graph.
	std::vector<std::unique_ptr<CoreNode>> nodes;
	std::vector<std::unique_ptr<Link>> links;
	
	friend class BlueprintSerializer;
};
