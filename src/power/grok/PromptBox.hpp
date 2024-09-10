#pragma once

#include <nanogui/nanogui.h>

class PromptBox : public nanogui::Window {
public:
	// Constructor initializes the chatbox and sets it up with a Nanogui screen
	PromptBox(nanogui::Widget& scene);
		
private:	
	// Text box for inputting new messages
	nanogui::TextBox* mInputBox;
	
	// Method to handle sending of messages
	void send_message();
};
