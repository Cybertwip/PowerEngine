/*
 * =====================================================================================
 *
 * Filename:  PrintNode.hpp
 *
 * Description:  Header file for the PrintNode class, which provides a
 * blueprint node for printing a string to the console.
 *
 * =====================================================================================
 */
#pragma once

#include "BlueprintNode.hpp"

#include <nanogui/textbox.h>

#include <memory>
#include <string>

// Forward declaration
class BlueprintCanvas;

/**
 * @class PrintCoreNode
 * @brief The logical part of a "Print String" node in the blueprint system.
 *
 * This node takes a string input and prints it to the console when the
 * execution flow reaches it. It has both input and output flow pins.
 */
class PrintCoreNode : public CoreNode {
public:
	/**
	 * @brief Constructs a PrintCoreNode with a specific UUID.
	 * @param id The unique identifier for this node.
	 */
	explicit PrintCoreNode(UUID id);
	
	/**
	 * @brief Executes the node's logic: printing a string to the console.
	 * @return True to allow the execution flow to continue.
	 */
	bool evaluate() override;
	
private:
	// Allow visual node to access private members to set the textbox pointer.
	friend class PrintVisualNode;
	
	CorePin& mStringInput; // The input pin for the string to be printed.
	nanogui::TextBox* mTextBox = nullptr; // A non-owning pointer to the UI element for fallback text.
};

/**
 * @class PrintVisualNode
 * @brief The visual representation of a "Print String" node.
 *
 * This class creates the NanoGUI window and a TextBox for default input.
 */
class PrintVisualNode : public VisualBlueprintNode {
public:
	/**
	 * @brief Constructs a PrintVisualNode.
	 * @param parent The BlueprintCanvas that owns this node.
	 * @param position The initial position of the node on the canvas.
	 * @param size The size of the node window.
	 * @param coreNode A reference to the logical core node that this visual node represents.
	 */
	PrintVisualNode(BlueprintCanvas& parent, nanogui::Vector2i position, nanogui::Vector2i size, PrintCoreNode& coreNode);
};

