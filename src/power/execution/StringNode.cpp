#include "StringNode.hpp"

namespace {
class StringPin : public Pin {
public:
	StringPin(nanogui::Widget& parent, int id, int node_id, const std::string& label, PinType type, PinSubType subtype, nanogui::TextBox& textbox)
	: Pin(parent, id, node_id, label, type, subtype)
	, mTextBox(textbox) {
		mTextBox.set_placeholder("Enter Text");
	}
	
	std::optional<std::variant<Entity, std::string, int, float, bool>> get_data() override {
		return mTextBox.value();
	}
	
	void set_data(std::optional<std::variant<Entity, std::string, int, float, bool>> data) override {
		if (data.has_value()) {
			mTextBox.set_value(std::get<std::string>(*data));
		} else {
			mTextBox.set_value("");
		}
	}
	
private:
	nanogui::TextBox& mTextBox;
};

}

StringNode::StringNode(std::optional<std::reference_wrapper<BlueprintCanvas>> parent, nanogui::Vector2i size, std::function<int()> id_registrator_lambda)
: BlueprintNode(parent, NodeType::String, "String", size, id_registrator_lambda(), nanogui::Color(255, 0, 255, 255)) {
	
	// Text box inside the node data wrapper: Will be centered due to the vertical alignment
	auto& textbox = add_data_widget<nanogui::TextBox>("");
	
	textbox.set_editable(true);
	textbox.set_alignment(nanogui::TextBox::Alignment::Left);
	
	auto& output = add_output<StringPin>(id_registrator_lambda(), this->id, "", PinType::String, PinSubType::None, textbox);
	
	link = [&output](){
		output.can_flow = true;
	};
}
