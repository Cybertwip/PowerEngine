#include "StringNode.hpp"

#include "execution/BlueprintCanvas.hpp" // For on_modified()

StringCoreNode::StringCoreNode(UUID id)
: DataCoreNode(NodeType::String, id, nanogui::Color(200, 50, 190, 255))
, mValue("Default")
{
    // A data node just needs to provide an output pin for other nodes to connect to.
    add_output(PinType::String, PinSubType::None);
}

std::optional<std::variant<Entity, std::string, long, float, bool>> StringCoreNode::get_data() const {
    // Return this node's internal value, wrapped in the required types.
    return mValue;
}

void StringCoreNode::set_data(std::optional<std::variant<Entity, std::string, long, float, bool>> data) {
    // Update this node's internal value from the variant.
    if (data.has_value() && std::holds_alternative<std::string>(*data)) {
        mValue = std::get<std::string>(*data);
    } else {
        mValue = ""; // Reset to a default state if data is invalid.
    }
}

StringVisualNode::StringVisualNode(BlueprintCanvas& parent, nanogui::Vector2i position, nanogui::Vector2i size, StringCoreNode& coreNode)
: VisualBlueprintNode(parent, "String", position, size, coreNode)
, mTextBox(add_data_widget<nanogui::TextBox>(""))
, mCoreNode(coreNode) {
    
    mTextBox.set_placeholder("Enter Text");
    mTextBox.set_editable(true);

    // When the node is created, sync the UI with the core node's current data.
    // This handles loading values from a file.
    auto data = mCoreNode.get_data();
    if (data.has_value()) {
        mTextBox.set_value(std::get<std::string>(*data));
    }
    
    // When the user types in the box, update the core data model.
    mTextBox.set_callback([this](const std::string& value) {
        mCoreNode.set_data(value);
        mCanvas.on_modified(); // Notify that the blueprint has changed.
        return true;
    });
}
