#include "PowerSettingsWindow.hpp"

#include "ai/PowerAi.hpp"

#include <nanogui/layout.h>
#include <nanogui/theme.h>
#include <nanogui/button.h>
#include <nanogui/label.h>
#include <nanogui/textbox.h>

#include <fstream>
#include <iostream>
#include <regex>

PowerSettingsWindow::PowerSettingsWindow(nanogui::Screen& parent, PowerAi& powerAi, std::function<void()> successCallback)
: nanogui::Window(parent, "Power AI Settings"),
data_saved_(false),
mSuccessCallback(successCallback),
mPowerAi(powerAi)
{
	// Configure window properties
	set_position(nanogui::Vector2i(10, 10));
	set_layout(std::make_unique<nanogui::GroupLayout>());
	set_fixed_size(nanogui::Vector2i(400, 400));
	set_visible(false); // Initially hidden
	
	// ----- OpenAI API Key -----
	m_api_key_label_ = std::make_shared<nanogui::Label>(*this, "OpenAI API Key:", "sans-bold");
	m_api_key_label_->set_font_size(14);
	m_api_key_label_->set_fixed_width(120);
	
	api_key_box_ = std::make_shared<nanogui::TextBox>(*this, "");
	api_key_box_->set_editable(true);
	api_key_box_->set_placeholder("Enter your OpenAI API key");
	api_key_box_->set_fixed_width(250);
	api_key_box_->set_password_character('*'); // Mask the input
	api_key_box_->set_callback([this](const std::string& value) -> bool {
		// Basic validation: API key length (example, adjust as needed)
		return value.length() > 20;
	});
	
	// ----- Tripo AI API Key -----
	m_tripo_ai_key_label_ = std::make_shared<nanogui::Label>(*this, "Tripo AI API Key:", "sans-bold");
	m_tripo_ai_key_label_->set_font_size(14);
	m_tripo_ai_key_label_->set_fixed_width(120);
	
	tripo_ai_key_box_ = std::make_shared<nanogui::TextBox>(*this, "");
	tripo_ai_key_box_->set_editable(true);
	tripo_ai_key_box_->set_placeholder("Enter your Tripo AI API key");
	tripo_ai_key_box_->set_fixed_width(250);
	tripo_ai_key_box_->set_password_character('*'); // Mask the input
	tripo_ai_key_box_->set_callback([this](const std::string& value) -> bool {
		// Basic validation: API key length (example, adjust as needed)
		return value.length() > 20;
	});
	
	// ----- DeepMotion Settings -----
	// API Base URL
	m_deepmotion_url_label_ = std::make_shared<nanogui::Label>(*this, "DeepMotion API Base URL:", "sans-bold");
	m_deepmotion_url_label_->set_font_size(14);
	m_deepmotion_url_label_->set_fixed_width(150);
	
	deepmotion_url_box_ = std::make_shared<nanogui::TextBox>(*this, "");
	deepmotion_url_box_->set_editable(true);
	deepmotion_url_box_->set_placeholder("e.g., api.deepmotion.com");
	deepmotion_url_box_->set_fixed_width(230);
	deepmotion_url_box_->set_callback([this](const std::string& value) -> bool {
		// Basic validation: URL format (simple regex)
		std::regex url_regex(R"(^(https?://)?([\w.-]+)(:\d+)?(/.*)?$)");
		return std::regex_match(value, url_regex);
	});
	
	// API Base Port
	m_deepmotion_port_label_ = std::make_shared<nanogui::Label>(*this, "DeepMotion API Port:", "sans-bold");
	m_deepmotion_port_label_->set_font_size(14);
	m_deepmotion_port_label_->set_fixed_width(150);
	
	deepmotion_port_box_ = std::make_shared<nanogui::TextBox>(*this, "443");
	deepmotion_port_box_->set_editable(true);
	deepmotion_port_box_->set_placeholder("Enter API Port");
	deepmotion_port_box_->set_fixed_width(230);
	deepmotion_port_box_->set_callback([this](const std::string& value) -> bool {
		// Validate if the port is numeric and within valid range
		if (value.empty()) return false;
		for (char c : value) {
			if (!isdigit(c)) return false;
		}
		int port = std::stoi(value);
		return port > 0 && port <= 65535;
	});
	
	// Client ID
	m_deepmotion_client_id_label_ = std::make_shared<nanogui::Label>(*this, "DeepMotion Client ID:", "sans-bold");
	m_deepmotion_client_id_label_->set_font_size(14);
	m_deepmotion_client_id_label_->set_fixed_width(150);
	
	deepmotion_client_id_box_ = std::make_shared<nanogui::TextBox>(*this, "");
	deepmotion_client_id_box_->set_editable(true);
	deepmotion_client_id_box_->set_placeholder("Enter Client ID");
	deepmotion_client_id_box_->set_fixed_width(230);
	deepmotion_client_id_box_->set_password_character('*'); // Mask the input
	deepmotion_client_id_box_->set_callback([this](const std::string& value) -> bool {
		// Basic validation
		return !value.empty();
	});
	
	// Client Secret
	m_deepmotion_client_secret_label_ = std::make_shared<nanogui::Label>(*this, "DeepMotion Client Secret:", "sans-bold");
	m_deepmotion_client_secret_label_->set_font_size(14);
	m_deepmotion_client_secret_label_->set_fixed_width(150);
	
	deepmotion_client_secret_box_ = std::make_shared<nanogui::TextBox>(*this, "");
	deepmotion_client_secret_box_->set_editable(true);
	deepmotion_client_secret_box_->set_placeholder("Enter Client Secret");
	deepmotion_client_secret_box_->set_fixed_width(230);
	deepmotion_client_secret_box_->set_password_character('*'); // Mask the input
	deepmotion_client_secret_box_->set_callback([this](const std::string& value) -> bool {
		// Basic validation
		return !value.empty();
	});
	
	// ----- Status Label -----
	status_label_ = std::make_shared<nanogui::Label>(*this, "", "sans");
	status_label_->set_fixed_size(nanogui::Vector2i(380, 20));
	status_label_->set_color(nanogui::Color(255, 255, 255, 255)); // White
	
	// ----- Sync Button -----
	sync_button_ = std::make_shared<nanogui::Button>(*this, "Sync");
	sync_button_->set_fixed_size(nanogui::Vector2i(80, 30));
	sync_button_->set_callback([this]() {
		this->on_sync();
	});
	
	// ----- Close Button -----
	close_button_ = std::make_shared<nanogui::Button>(*this, "Close");
	close_button_->set_fixed_size(nanogui::Vector2i(80, 30));
	close_button_->set_callback([this]() {
		this->set_visible(false);
	});
	
	// ----- Button Panel Layout -----
	m_button_panel_ = std::make_shared<nanogui::Widget>(std::make_optional<std::reference_wrapper<nanogui::Widget>>(*this));
	m_button_panel_->set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Horizontal, nanogui::Alignment::Middle, 10, 0));
	m_button_panel_->add_child(*sync_button_);
	m_button_panel_->add_child(*close_button_);
	
	// Remove child from the window layout to avoid duplication
	remove_child(*sync_button_);
	remove_child(*close_button_);
	
	// ----- Attempt to Load Existing Credentials and Authenticate -----
	if (load_from_file("power_ai_credentials.dat")) {
		// Populate the text boxes with loaded data
		api_key_box_->set_value(api_key_);
		tripo_ai_key_box_->set_value(tripo_ai_key_);
		deepmotion_url_box_->set_value(deepmotion_api_base_url_);
		deepmotion_port_box_->set_value(deepmotion_api_base_port_);
		deepmotion_client_id_box_->set_value(deepmotion_client_id_);
		deepmotion_client_secret_box_->set_value(deepmotion_client_secret_);
	}
}

/**
 * @brief Handles the synchronization/authentication process.
 */
void PowerSettingsWindow::on_sync() {
	std::string openai_api_key_input = api_key_box_->value();
	std::string tripo_ai_api_key_input = tripo_ai_key_box_->value();
	std::string deepmotion_api_base_url_input = deepmotion_url_box_->value();
	std::string deepmotion_api_base_port_input = deepmotion_port_box_->value();
	std::string deepmotion_client_id_input = deepmotion_client_id_box_->value();
	std::string deepmotion_client_secret_input = deepmotion_client_secret_box_->value();
	
	// Validate inputs
	if (openai_api_key_input.empty() || tripo_ai_api_key_input.empty() ||
		deepmotion_api_base_url_input.empty() || deepmotion_api_base_port_input.empty() ||
		deepmotion_client_id_input.empty() || deepmotion_client_secret_input.empty()) {
		status_label_->set_caption("All fields must be filled.");
		status_label_->set_color(nanogui::Color(255, 0, 0, 255)); // Red color
		return;
	}
	
	// Parse DeepMotion API Port
	int deepmotion_api_base_port = 443; // Default port
	try {
		deepmotion_api_base_port = std::stoi(deepmotion_api_base_port_input);
		if (deepmotion_api_base_port <= 0 || deepmotion_api_base_port > 65535) {
			throw std::out_of_range("Port number out of valid range.");
		}
	} catch (...) {
		status_label_->set_caption("Invalid DeepMotion API Port.");
		status_label_->set_color(nanogui::Color(255, 0, 0, 255)); // Red color
		return;
	}
	
	// Update status label to indicate authentication is in progress
	status_label_->set_caption("Status: Authenticating...");
	status_label_->set_color(nanogui::Color(255, 255, 0, 255)); // Yellow color
	
	// Call PowerAi's authenticate_async
	mPowerAi.authenticate_async(
								openai_api_key_input,
								tripo_ai_api_key_input,
								deepmotion_api_base_url_input,
								deepmotion_api_base_port,
								deepmotion_client_id_input,
								deepmotion_client_secret_input,
								[this](bool success, const std::string& error_message) {
									// Ensure UI updates are performed on the main thread
									nanogui::async([this, success, error_message]() {
										if (success) {
											// Update status label to success
											status_label_->set_caption("Authentication successful.");
											status_label_->set_color(nanogui::Color(0, 255, 0, 255)); // Green color
											
											// Save the credentials
											if (save_to_file("power_ai_credentials.dat",
															 api_key_box_->value(),
															 tripo_ai_key_box_->value(),
															 deepmotion_url_box_->value(),
															 deepmotion_port_box_->value(),
															 deepmotion_client_id_box_->value(),
															 deepmotion_client_secret_box_->value())) {
												data_saved_ = true;
												std::cout << "Credentials saved successfully." << std::endl;
											}
											
											// Invoke the success callback
											if (mSuccessCallback) {
												mSuccessCallback();
											}
											
											// Optionally, close the settings window
											this->set_visible(false);
										} else {
											// Authentication failed
											status_label_->set_caption("Authentication failed:\n" + error_message);
											status_label_->set_color(nanogui::Color(255, 0, 0, 255)); // Red color
											data_saved_ = false;
										}
									});
								}
								);
}

/**
 * @brief Loads credentials from a file.
 */
bool PowerSettingsWindow::load_from_file(const std::string& filename) {
	std::ifstream file(filename, std::ios::in);
	if (!file.is_open()) {
		// File does not exist
		return false;
	}
	
	std::string line;
	while (std::getline(file, line)) {
		// Remove any trailing carriage return characters (for Windows compatibility)
		if (!line.empty() && line.back() == '\r') {
			line.pop_back();
		}
		
		std::size_t delimiter_pos = line.find(':');
		if (delimiter_pos == std::string::npos) {
			continue; // Invalid line format
		}
		
		std::string key = line.substr(0, delimiter_pos);
		std::string value = line.substr(delimiter_pos + 1);
		
		// Trim whitespace from key and value
		key.erase(0, key.find_first_not_of(" \t\n\r"));
		key.erase(key.find_last_not_of(" \t\n\r") + 1);
		value.erase(0, value.find_first_not_of(" \t\n\r"));
		value.erase(value.find_last_not_of(" \t\n\r") + 1);
		
		if (key == "OpenAI_API_Key") {
			api_key_ = value;
		} else if (key == "TripoAI_API_Key") {
			tripo_ai_key_ = value;
		} else if (key == "DeepMotion_API_Base_URL") {
			deepmotion_api_base_url_ = value;
		} else if (key == "DeepMotion_API_Base_Port") {
			deepmotion_api_base_port_ = value;
		} else if (key == "DeepMotion_Client_ID") {
			deepmotion_client_id_ = value;
		} else if (key == "DeepMotion_Client_Secret") {
			deepmotion_client_secret_ = value;
		}
	}
	
	file.close();
	
	// Check if all necessary fields are loaded
	if (api_key_.empty() || tripo_ai_key_.empty() ||
		deepmotion_api_base_url_.empty() || deepmotion_api_base_port_.empty() ||
		deepmotion_client_id_.empty() || deepmotion_client_secret_.empty()) {
		// Incomplete data
		return false;
	}
	
	return true;
}

/**
 * @brief Saves credentials to a file.
 */
bool PowerSettingsWindow::save_to_file(const std::string& filename,
									   const std::string& api_key,
									   const std::string& tripo_ai_key,
									   const std::string& deepmotion_api_base_url,
									   const std::string& deepmotion_api_base_port,
									   const std::string& deepmotion_client_id,
									   const std::string& deepmotion_client_secret) {
	std::ofstream file(filename, std::ios::out | std::ios::binary);
	if (file.is_open()) {
		file << "OpenAI_API_Key:" << api_key << "\n";
		file << "TripoAI_API_Key:" << tripo_ai_key << "\n";
		file << "DeepMotion_API_Base_URL:" << deepmotion_api_base_url << "\n";
		file << "DeepMotion_API_Base_Port:" << deepmotion_api_base_port << "\n";
		file << "DeepMotion_Client_ID:" << deepmotion_client_id << "\n";
		file << "DeepMotion_Client_Secret:" << deepmotion_client_secret << "\n";
		file.close();
		std::cout << "Credentials saved to " << filename << std::endl;
		
		return true;
	} else {
		// Handle file opening error
		status_label_->set_caption("Failed to save credentials.");
		status_label_->set_color(nanogui::Color(255, 0, 0, 255)); // Red color
		
		return false;
	}
}
