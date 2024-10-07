#pragma once

#include <nanogui/nanogui.h>

class PromptBox : public nanogui::Window {
public:
	// Constructor initializes the chatbox and sets it up with a Nanogui screen
	PromptBox(std::weak_ptr<nanogui::Widget> scene);
		
private:
	void initialize() override;
	
	// Text box for inputting new messages
	std::shared_ptr<nanogui::TextBox> mInputBox;
	
	// Method to handle sending of messages
	void send_message();
};
