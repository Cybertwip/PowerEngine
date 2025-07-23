/*
 * =====================================================================================
 *
 * Filename:  StringNode.hpp
 *
 * Description:  Header file for the StringNode class, which provides a
 * blueprint node for handling string data.
 *
 * =====================================================================================
 */
#pragma once

#include "BlueprintNode.hpp" // The main header from the previous step

#include <nanogui/textbox.h>

#include <memory>
#include <string>

// Forward declaration
class BlueprintCanvas;

/**
 * @class StringCoreNode
 * @brief The logical part of a string node in the blueprint system.
 *
 * This class inherits from DataCoreNode and manages the storage and retrieval
 * of a string value. It provides an output pin so other nodes can access its data.
 */
class StringCoreNode : public DataCoreNode {
public:
	/**
	 * @brief Constructs a StringCoreNode with a specific UUID.
	 * @param id The unique identifier for this node.
	 */
	explicit StringCoreNode(UUID id);
	
	/**
	 * @brief Sets the node's internal string value from a std::any object.
	 * @param data An optional std::any containing the new string value.
	 */
	void set_data(std::optional<std::any> data) override;
	
	/**
	 * @brief Gets the node's internal string value, wrapped in a std::any.
	 * @return An optional std::any containing the current string value.
	 */
	std::optional<std::any> get_data() const override;
	
private:
	std::string mValue; // The internal storage for the string data.
};

/**
 * @class StringVisualNode
 * @brief The visual representation of a string node in the blueprint editor.
 *
 * This class creates the NanoGUI window and widgets (like a TextBox) for
 * interacting with a StringCoreNode. It syncs the UI with the underlying data.
 */
class StringVisualNode : public VisualBlueprintNode {
public:
	/**
	 * @brief Constructs a StringVisualNode.
	 * @param parent The BlueprintCanvas that owns this node.
	 * @param position The initial position of the node on the canvas.
	 * @param size The size of the node window.
	 * @param coreNode A reference to the logical core node that this visual node represents.
	 */
	StringVisualNode(BlueprintCanvas& parent, nanogui::Vector2i position, nanogui::Vector2i size, StringCoreNode& coreNode);
	
private:
	nanogui::TextBox& mTextBox; // The text input widget.
	StringCoreNode& mCoreNode;  // A reference back to the core data node.
};


