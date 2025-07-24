
/*
 * =====================================================================================
 *
 * Filename:  PrintNode.cpp
 *
 * Description:  Implementation file for the PrintNode class.
 *
 * =====================================================================================
 */

// #include "PrintNode.hpp" // This would be included in a real project structure
#include "execution/BlueprintCanvas.hpp" // For mCanvas.on_modified()
#include <iostream>

PrintCoreNode::PrintCoreNode(UUID id)
: CoreNode(NodeType::Print, id, nanogui::Color(130, 200, 255, 255)),
// The PinSubType is no longer needed in the constructor.
mStringInput(add_input(PinType::String))
{
	// Add the flow pins
	add_input(PinType::Flow);
	add_output(PinType::Flow);
}

bool PrintCoreNode::evaluate(UUID flow_pin_id) {
	std::string string_to_print;
	bool data_found = false;
	
	// The framework has already populated the input pin's data.
	auto data = mStringInput.get_data();
	
	// Use the data if the pin is connected and the data is a valid string.
	if (data.has_value()) {
		// Safely cast the std::any to a string pointer to check the type.
		if (const std::string* pval = std::any_cast<std::string>(&(*data))) {
			string_to_print = *pval;
			data_found = true;
		}
	}
	
	// Otherwise, use the local text box as a fallback (for unconnected pins or wrong data types).
	if (!data_found && mTextBox) {
		string_to_print = mTextBox->value();
	}
	
	// The node's primary action: print the string.
	std::cout << "Blueprint Print: " << string_to_print << std::endl;
	
	// Return true to allow the execution flow to continue to the next node.
	return true;
}


PrintVisualNode::PrintVisualNode(BlueprintCanvas& parent, nanogui::Vector2i position, nanogui::Vector2i size, PrintCoreNode& coreNode)
: VisualBlueprintNode(parent, "Print String", position, size, coreNode) {
	
	// Add a TextBox widget to the node's UI for default string input.
	auto& textbox = add_data_widget<nanogui::TextBox>();
	textbox.set_value("Hello, world!");
	textbox.set_editable(true);
	
	// Give the core node a pointer to the textbox to get its value if the input pin is not connected.
	coreNode.mTextBox = &textbox;
	
	// When the user changes the text, notify the canvas so the change can be saved.
	textbox.set_callback([this](const std::string& /*unused*/) {
		mCanvas.on_modified();
		return true;
	});
}
