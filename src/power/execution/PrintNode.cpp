#include "PrintNode.hpp"

#include "execution/BlueprintCanvas.hpp" // For mCanvas.on_modified()
#include <iostream>

PrintCoreNode::PrintCoreNode(UUID id)
: CoreNode(NodeType::Print, id, nanogui::Color(130, 200, 255, 255))
, mStringInput(add_input(PinType::String, PinSubType::None))
{
    // Add the flow pins
    add_input(PinType::Flow, PinSubType::None);
    add_output(PinType::Flow, PinSubType::None);
}

bool PrintCoreNode::evaluate() {
    std::string string_to_print;

    // The framework has already populated the input pin's data.
    auto data = mStringInput.get_data();

    // Use the data if the pin is connected and the data is valid.
    if (data.has_value() && std::holds_alternative<std::string>(*data)) {
	        string_to_print = std::get<std::string>(*data);
	    }
    // Otherwise, use the local text box as a fallback (for unconnected pins).
    else if (mTextBox) {
	        string_to_print = mTextBox->value();
	    }

    // The node's primary action: print the string.
    std::cout << "Blueprint Print: " << string_to_print << std::endl;

    // Return true to allow the execution flow to continue.
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
