#include "PromptBox.hpp"

PromptBox::PromptBox(nanogui::Widget& parent)
: nanogui::Window(parent, "Grok Prompt") {
	// Create a vertical layout for the chatbox
	set_layout(std::make_unique<nanogui::GroupLayout>());
	
	// Create an input box for typing new messages
	mInputBox = std::make_shared<nanogui::TextBox>(*this, "");
	mInputBox->set_editable(true);
	mInputBox->set_placeholder("Type your prompt...");
	mInputBox->set_callback([this](const std::string& text) {
		if (!text.empty()) {
			send_message();
		}
		return true;
	});
}

void PromptBox::send_message() {
	std::string message = mInputBox->value();
	if (!message.empty()) {
		mInputBox->set_value("");  // Clear the input box
	}
}
