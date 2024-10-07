// DeepMotionSettingsWindow.cpp

#include "DeepMotionSettingsWindow.hpp"
#include "DeepMotionApiClient.hpp"

#include "filesystem/UrlOpener.hpp"

#include <nanogui/layout.h>
#include <nanogui/theme.h>
#include <nanogui/imageview.h> // Added for nanogui::ImageView
#include <nanogui/texture.h>    // Added for nanogui::Texture

#include <httplib.h>
#include <json/json.h>

#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <regex>

DeepMotionSettingsWindow::DeepMotionSettingsWindow(std::shared_ptr<nanogui::Screen> screen, DeepMotionApiClient& deepMotionApiClient, std::function<void()> successCallback)
: nanogui::Window(screen),
data_saved_(false),
mSuccessCallback(successCallback),
mDeepMotionApiClient(deepMotionApiClient)
{
	set_visible(false);
	set_modal(false);
	// Window configuration to mimic ImGui flags
	set_fixed_size(nanogui::Vector2i(400, 320));
	set_layout(std::make_shared<nanogui::GroupLayout>());
	set_title("Sync With DeepMotion");
}

void DeepMotionSettingsWindow::initialize() {
	// Close Button
	mCloseButton = std::make_shared<nanogui::Button>(button_panel(), "X");
	mCloseButton->set_fixed_size(nanogui::Vector2i(20, 20));
	mCloseButton->set_callback([this]() {
		this->set_visible(false);
		this->set_modal(false);
	});
	
	// Position the close button at the top-right corner
	// Using a horizontal BoxLayout with a spacer
	mTopPanel = std::make_shared<nanogui::Widget>(shared_from_this());
	
	mTopPanel->set_layout(std::make_shared<nanogui::BoxLayout>(nanogui::Orientation::Horizontal,
															   nanogui::Alignment::Middle, 0, 0));
	// API Base URL Input
	mApiBaseLabel = std::make_shared<nanogui::Label>(shared_from_this(), "API Base URL:", "sans-bold");
	api_base_url_box_ = std::make_shared<nanogui::TextBox>(shared_from_this(), "");
	api_base_url_box_->set_placeholder("Enter API Base URL");
	api_base_url_box_->set_editable(true);
	api_base_url_box_->set_fixed_width(350);
	api_base_url_box_->set_callback([this](const std::string& value) {
		// Basic validation can be added here if necessary
		return true;
	});
	
	// Client ID Input
	mClientIdLabel = std::make_shared<nanogui::Label>(shared_from_this(), "Client ID:", "sans-bold");
	client_id_box_ = std::make_shared<nanogui::TextBox>(shared_from_this(), "");
	client_id_box_->set_placeholder("Enter Client ID");
	client_id_box_->set_editable(true);
	client_id_box_->set_password_character('*');
	client_id_box_->set_fixed_width(350);
	client_id_box_->set_callback([this](const std::string& value) {
		client_id_ = value;
		return true;
	});
	
	// Client Secret Input
	mClientSecretLabel = std::make_shared<nanogui::Label>(shared_from_this(), "Client Secret:", "sans-bold");
	client_secret_box_ = std::make_shared<nanogui::TextBox>(shared_from_this(), "");
	client_secret_box_->set_placeholder("Enter Client Secret");
	client_secret_box_->set_editable(true);
	client_secret_box_->set_password_character('*');
	client_secret_box_->set_fixed_width(350);
	client_secret_box_->set_callback([this](const std::string& value) {
		client_secret_ = value;
		return true;
	});
	
	// Sync Button
	mSyncButton = std::make_shared<nanogui::Button>(shared_from_this(), "Sync");
	mSyncButton->set_fixed_size(nanogui::Vector2i(96, 48));
	mSyncButton->set_callback([this]() {
		this->on_sync();
	});
	
	// Status Panel
	mStatusPanel = std::make_shared<nanogui::Widget>(shared_from_this());
	mStatusPanel->set_layout(std::make_shared<nanogui::GridLayout>(
																   nanogui::Orientation::Horizontal, // Layout orientation
																   2,                               // Number of columns
																   nanogui::Alignment::Maximum,     // Alignment within cells
																   0,                               // Column padding
																   0                                // Row padding
																   ));
	
	// Status Label
	mStatusLabel = std::make_shared<nanogui::Label>(mStatusPanel, "", "sans");
	mStatusLabel->set_fixed_size(nanogui::Vector2i(175, 20));
	mStatusLabel->set_color(nanogui::Color(255, 255, 255, 255)); // White color
	
	// DeepMotion Button with Image
	mDeepMotionButton = std::make_shared<nanogui::Button>(mStatusPanel, "");
	mDeepMotionButton->set_fixed_size(nanogui::Vector2i(48, 48));
	mDeepMotionButton->set_callback([this]() {
		UrlOpener::openUrl("https://deepmotion.com/");
	});
	
	mImageView = std::make_shared<nanogui::ImageView>(mDeepMotionButton);
	mImageView->set_size(nanogui::Vector2i(48, 48));
	
	mImageView->set_fixed_size(mImageView->size());
	
	mImageView->set_image(std::make_shared<nanogui::Texture>("internal/ui/poweredby.png",
															 nanogui::Texture::InterpolationMode::Bilinear,
															 nanogui::Texture::InterpolationMode::Nearest,
															 nanogui::Texture::WrapMode::Repeat));
	
	mImageView->set_visible(true);
	
	mImageView->image()->resize(nanogui::Vector2i(96, 96));
	
	// Initially hide the window
	// set_visible(false);
	// set_modal(true);
	
	// Attempt to load existing credentials and synchronize
	if (load_from_file("powerkey.dat")) {
		// Populate the text boxes with loaded data
		api_base_url_box_->set_value(api_base_url_);
		client_id_box_->set_value(client_id_);
		client_secret_box_->set_value(client_secret_);
		
		// Perform synchronization
		on_sync();
	}
	
	nanogui::Window::initialize();

}

void DeepMotionSettingsWindow::on_sync() {
	// Retrieve input values
	std::string api_base_url_input = api_base_url_box_->value();
	int api_base_port = 443; // Default port
	
	// Regex to extract host and optional port
	std::regex url_regex(R"(^(?:https?://)?([^:/\s]+)(?::(\d+))?)");
	std::smatch matches;
	
	if (std::regex_match(api_base_url_input, matches, url_regex)) {
		std::string host = matches[1].str();
		if (matches[2].matched) {
			try {
				api_base_port = std::stoi(matches[2].str());
				// Add port range validation here if necessary
			} catch (...) {
				api_base_port = 443; // Default port
			}
		} else {
			api_base_port = 443; // Default port
		}
		api_base_url_ = host;
	} else {
		// Handle invalid URL format
		api_base_url_ = "";
		api_base_port = 443;
	}
	
	std::string client_id = client_id_box_->value();
	std::string client_secret = client_secret_box_->value();
	
	// Validate input
	if (api_base_url_.empty() || client_id.empty() || client_secret.empty()) {
		mStatusLabel->set_caption("API Base URL, Client ID, and Secret cannot be empty.");
		mStatusLabel->set_color(nanogui::Color(255, 0, 0, 255)); // Red color
		return;
	}
	
	// Disable the Sync button to prevent multiple clicks
	// Optionally, you can pass a reference to the Sync button to disable it here
	// For simplicity, assume 'sync_button' is accessible. If not, consider making it a member variable.
	
	// Update status label to indicate authentication is in progress
	mStatusLabel->set_caption("Status: Authenticating...");
	mStatusLabel->set_color(nanogui::Color(255, 255, 0, 255)); // Yellow color
	
	// Call the asynchronous authenticate method
	mDeepMotionApiClient.authenticate_async(api_base_url_, api_base_port, client_id, client_secret,
											[this, api_base_port](bool success, const std::string& error_message) {
		// Ensure UI updates are performed on the main thread
		nanogui::async([this, success, error_message, api_base_port]() {
			if (success) {
				// Update status label to success
				mStatusLabel->set_caption("Synchronization successful.");
				mStatusLabel->set_color(nanogui::Color(0, 255, 0, 255)); // Green color
				
				// Save credentials and API Base URL if not already saved
				if (!data_saved_) {
					save_to_file("powerkey.dat", api_base_url_, std::to_string(api_base_port), client_id_, client_secret_);
					data_saved_ = true;
				}
				
				// Hide the window if visible
				if (visible()) {
					set_visible(false);
					set_modal(false);
					
					// Call the success callback
					if(mSuccessCallback) {
						mSuccessCallback();
					}
				}
				
			} else {
				// Authentication failed
				mStatusLabel->set_caption("Invalid credentials or server error.");
				mStatusLabel->set_color(nanogui::Color(255, 0, 0, 255)); // Red color
				data_saved_ = false;
			}
		});
	}
											);
}

bool DeepMotionSettingsWindow::load_from_file(const std::string& filename) {
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
		
		if (key == "APIBaseURL") {
			api_base_url_ = value;
		} else if (key == "APIBasePort") {
			try {
				api_base_port_ = std::stoi(value);
			} catch (...) {
				api_base_port_ = 443; // Default port
			}
		} else if (key == "ClientID") {
			client_id_ = value;
		} else if (key == "ClientSecret") {
			client_secret_ = value;
		}
	}
	
	file.close();
	
	// Check if all necessary fields are loaded
	if (api_base_url_.empty() || client_id_.empty() || client_secret_.empty()) {
		// Incomplete data
		return false;
	}
	
	return true;
}

void DeepMotionSettingsWindow::save_to_file(const std::string& filename, const std::string& api_base_url, const std::string& api_base_port, const std::string& client_id, const std::string& client_secret) {
	std::ofstream file(filename, std::ios::out | std::ios::binary);
	if (file.is_open()) {
		file << "APIBaseURL:" << api_base_url << "\n";
		file << "APIBasePort:" << api_base_port << "\n";
		file << "ClientID:" << client_id << "\n";
		file << "ClientSecret:" << client_secret << "\n";
		file.close();
	} else {
		// Handle file opening error
		mStatusLabel->set_caption("Failed to save credentials.");
		mStatusLabel->set_color(nanogui::Color(255, 0, 0, 255)); // Red color
	}
}
