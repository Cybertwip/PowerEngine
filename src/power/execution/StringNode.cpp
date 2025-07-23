#include "StringNode.hpp"

#include "execution/BlueprintCanvas.hpp" // For on_modified()


StringCoreNode::StringCoreNode(UUID id)
: DataCoreNode(NodeType::String, id, nanogui::Color(200, 50, 190, 255)),
mValue("Default")
{
	// A data node just needs to provide an output pin for other nodes to connect to.
	// The PinSubType is no longer needed.
	add_output(PinType::String);
}

std::optional<std::any> StringCoreNode::get_data() const {
	// Return this node's internal value, wrapped in the required types.
	return mValue;
}

void StringCoreNode::set_data(std::optional<std::any> data) {
	// Update this node's internal value from the std::any.
	if (data.has_value()) {
		// Safely cast the std::any to a string pointer to check the type.
		if (const std::string* pval = std::any_cast<std::string>(&(*data))) {
			mValue = *pval;
		} else {
			mValue = ""; // Reset if the type is wrong.
		}
	} else {
		mValue = ""; // Reset to a default state if data is empty.
	}
}

StringVisualNode::StringVisualNode(BlueprintCanvas& parent, nanogui::Vector2i position, nanogui::Vector2i size, StringCoreNode& coreNode)
: VisualBlueprintNode(parent, "String", position, size, coreNode),
mTextBox(add_data_widget<nanogui::TextBox>("")),
mCoreNode(coreNode)
{
	mTextBox.set_placeholder("Enter Text");
	mTextBox.set_editable(true);
	
	// When the node is created, sync the UI with the core node's current data.
	// This handles loading values from a file.
	auto data = mCoreNode.get_data();
	if (data.has_value()) {
		// Safely cast the std::any to get the string value for the UI.
		if (const std::string* pval = std::any_cast<std::string>(&(*data))) {
			mTextBox.set_value(*pval);
		}
	}
	
	// When the user types in the box, update the core data model.
	mTextBox.set_callback([this](const std::string& value) {
		// The string value is automatically converted to std::any when passed to set_data.
		mCoreNode.set_data(value);
		mCanvas.on_modified(); // Notify that the blueprint has changed.
		return true;
	});
}
