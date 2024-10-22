#include "DallESettingsWindow.hpp"
#include "DallEApiClient.hpp" // Ensure this includes the updated DallEApiClient

#include <nanogui/layout.h>
#include <nanogui/theme.h>
#include <nanogui/button.h>
#include <nanogui/label.h>
#include <nanogui/textbox.h>

#include <fstream>
#include <iostream>
#include <regex>

DallESettingsWindow::DallESettingsWindow(nanogui::Screen& parent, DallEApiClient& dalleClient, std::function<void()> successCallback)
: nanogui::Window(parent, "DALL-E Settings"),
data_saved_(false),
mSuccessCallback(successCallback),
mDallEApiClient(dalleClient)
{
	// Configure window properties
	set_position(nanogui::Vector2i(10, 10));
	set_layout(std::make_unique<nanogui::GroupLayout>());
	set_fixed_size(nanogui::Vector2i(400, 200));
	set_visible(false); // Initially hidden
	
	// API Key Label
	m_api_key_label_ = std::make_shared<nanogui::Label>(*this, "OpenAI API Key:", "sans-bold");
	m_api_key_label_->set_font_size(14);
	m_api_key_label_->set_fixed_width(120);
	
	// API Key TextBox
	api_key_box_ = std::make_shared<nanogui::TextBox>(*this, "");
	api_key_box_->set_editable(true);
	api_key_box_->set_placeholder("Enter your OpenAI API key");
	api_key_box_->set_fixed_width(250);
	api_key_box_->set_password_character('*'); // Mask the input
	api_key_box_->set_callback([this](const std::string& value) -> bool {
		// Basic validation: API key length (example, adjust as needed)
		return value.length() > 20;
	});
	
	// Status Label
	status_label_ = std::make_shared<nanogui::Label>(*this, "", "sans");
	status_label_->set_fixed_size(nanogui::Vector2i(380, 20));
	status_label_->set_color(nanogui::Color(255, 255, 255, 255)); // White
	
	// Sync Button
	sync_button_ = std::make_shared<nanogui::Button>(*this, "Sync");
	sync_button_->set_fixed_size(nanogui::Vector2i(80, 30));
	sync_button_->set_callback([this]() {
		this->on_sync();
	});
	
	// Close Button
	close_button_ = std::make_shared<nanogui::Button>(*this, "Close");
	close_button_->set_fixed_size(nanogui::Vector2i(80, 30));
	close_button_->set_callback([this]() {
		this->set_visible(false);
	});
	
	// Layout for buttons
	m_button_panel_ = std::make_shared<nanogui::Widget>(std::optional<std::reference_wrapper<nanogui::Widget>>(*this));
	m_button_panel_->set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Horizontal, nanogui::Alignment::Middle, 10, 0));
	m_button_panel_->add_child(*sync_button_);
	m_button_panel_->add_child(*close_button_);
	
	remove_child(*sync_button);
	remove_child(*close_button_);
	
	// Attempt to load existing API key and authenticate
	if (load_from_file("dalle_api_key.dat")) {
		api_key_box_->set_value(mDallEApiClient.get_api_key()); // Assuming DallEApiClient has get_api_key()
		on_sync(); // Automatically authenticate
	}
}

void DallESettingsWindow::on_sync() {
	std::string api_key_input = api_key_box_->value();
	
	// Validate API key input (basic validation)
	if (api_key_input.empty()) {
		status_label_->set_caption("API key cannot be empty.");
		status_label_->set_color(nanogui::Color(255, 0, 0, 255)); // Red
		return;
	}
		
	// Update status label to indicate authentication is in progress
	status_label_->set_caption("Status: Authenticating...");
	status_label_->set_color(nanogui::Color(255, 255, 0, 255)); // Yellow
	
	// Call authenticate asynchronously
	mDallEApiClient.authenticate_async(api_key_input, [&](bool success, const std::string& error_message) {
		// Ensure UI updates are performed on the main thread
		nanogui::async([this, success, error_message, api_key_input]() {
			if (success) {
				// Update status label to success
				status_label_->set_caption("Authentication successful.");
				status_label_->set_color(nanogui::Color(0, 255, 0, 255)); // Green
				
				// Save the API key
				if (save_to_file("dalle_api_key.dat", api_key_input)) {
					data_saved_ = true;
					std::cout << "API key saved successfully." << std::endl;
				}
				
				// Invoke the success callback
				if (mSuccessCallback) {
					mSuccessCallback();
				}
				
				// Optionally, close the settings window
				this->set_visible(false);
			} else {
				// Authentication failed
				status_label_->set_caption("Authentication failed: " + error_message);
				status_label_->set_color(nanogui::Color(255, 0, 0, 255)); // Red
				data_saved_ = false;
			}
		});
	});
}

bool DallESettingsWindow::load_from_file(const std::string& filename) {
	std::ifstream file(filename, std::ios::in);
	if (!file.is_open()) {
		// File does not exist
		return false;
	}
	
	std::string api_key;
	std::getline(file, api_key);
	file.close();
	
	if (api_key.empty()) {
		return false;
	}
	
	// Assuming DallEApiClient has a method to set the API key directly
	// Alternatively, you can authenticate here
	return mDallEApiClient.authenticate(api_key);
}

bool DallESettingsWindow::save_to_file(const std::string& filename, const std::string& api_key) {
	std::ofstream file(filename, std::ios::out | std::ios::trunc);
	if (file.is_open()) {
		file << api_key;
		file.close();
		std::cout << "API key saved to " << filename << std::endl;
		
		return true;
	} else {
		// Handle file opening error
		status_label_->set_caption("Failed to save API key.");
		status_label_->set_color(nanogui::Color(255, 0, 0, 255)); // Red
		
		return false;
	}
}
