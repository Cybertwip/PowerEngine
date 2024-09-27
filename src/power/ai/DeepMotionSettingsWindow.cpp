// DeepMotionSettingsWindow.cpp

#include "DeepMotionSettingsWindow.hpp"

#include <nanogui/layout.h>
#include <nanogui/theme.h>

#include <httplib.h>
#include <json/json.h>

#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <regex>

// Base64 encoding implementation
static const std::string base64_chars =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";

static inline bool is_base64(uint8_t c) {
	return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string base64_encode(const unsigned char* bytes_to_encode, size_t in_len) {
	std::string ret;
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];
	
	size_t pos = 0;
	while (in_len--) {
		char_array_3[i++] = bytes_to_encode[pos++];
		if (i ==3) {
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) +
			((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) +
			((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;
			
			for(i = 0; (i <4) ; i++)
				ret += base64_chars[char_array_4[i]];
			i = 0;
		}
	}
	
	if (i)
	{
		for(j = i; j < 3; j++)
			char_array_3[j] = '\0';
		
		char_array_4[0] = ( char_array_3[0] & 0xfc ) >> 2;
		char_array_4[1] = ( ( char_array_3[0] & 0x03 ) << 4 ) + ( ( char_array_3[1] & 0xf0 ) >> 4 );
		char_array_4[2] = ( ( char_array_3[1] & 0x0f ) << 2 ) + ( ( char_array_3[2] & 0xc0 ) >> 6 );
		char_array_4[3] = char_array_3[2] & 0x3f;
		
		for (j = 0; (j < i + 1); j++)
			ret += base64_chars[char_array_4[j]];
		
		while((i++ < 3))
			ret += '=';
	}
	
	return ret;
}

DeepMotionSettingsWindow::DeepMotionSettingsWindow(nanogui::Widget* parent)
: nanogui::Window(parent->screen()),
is_visible_(false),
data_saved_(false)
{
	// Window configuration to mimic ImGui flags
	set_fixed_size(nanogui::Vector2i(400, 320));
	set_layout(new nanogui::GroupLayout());
	set_title("Sync With DeepMotion");
	
	// Close Button
	auto close_button = new nanogui::Button(button_panel(), "X");
	close_button->set_fixed_size(nanogui::Vector2i(20, 20));
	close_button->set_callback([this]() {
		this->set_visible(false);
		this->set_modal(false);
		is_visible_ = false;
	});
	
	// Position the close button at the top-right corner
	// Using a horizontal BoxLayout with a spacer
	auto top_panel = new nanogui::Widget(this);
	top_panel->set_layout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal,
												 nanogui::Alignment::Middle, 0, 0));
	
	// Spacer to push the close button to the right
	auto spacer = new nanogui::Widget(top_panel);
	spacer->set_fixed_size(nanogui::Vector2i(360, 0));
	
	
	auto imageView = new nanogui::ImageView(this);
	imageView->set_size(nanogui::Vector2i(256, 256));
	
	imageView->set_fixed_size(imageView->size());
	
	imageView->set_image(new nanogui::Texture(
											  "internal/ui/poweredby.png",
											  nanogui::Texture::InterpolationMode::Bilinear,
											  nanogui::Texture::InterpolationMode::Nearest,
											  nanogui::Texture::WrapMode::Repeat));
	
	imageView->image()->resize(nanogui::Vector2i(256, 256));
	
	imageView->set_visible(true);
	
	// Spacer for visual separation
	new nanogui::Label(this, "", "sans", 20);
	
	// API Base URL Input
	new nanogui::Label(this, "API Base URL:", "sans-bold");
	api_base_url_box_ = new nanogui::TextBox(this, "");
	api_base_url_box_->set_placeholder("Enter API Base URL");
	api_base_url_box_->set_editable(true);
	api_base_url_box_->set_fixed_width(350);
	api_base_url_box_->set_callback([this](const std::string& value) {
		// Basic validation can be added here if necessary
		return true;
	});
	
	// Spacer for visual separation
	new nanogui::Label(this, "", "sans", 20);
	
	// Client ID Input
	new nanogui::Label(this, "Client ID:", "sans-bold");
	client_id_box_ = new nanogui::TextBox(this, "");
	client_id_box_->set_placeholder("Enter Client ID");
	client_id_box_->set_editable(true);
	client_id_box_->set_password_character('*');
	client_id_box_->set_fixed_width(350);
	client_id_box_->set_callback([this](const std::string& value) {
		client_id_ = value;
		return true;
	});
	
	// Spacer
	new nanogui::Label(this, "", "sans", 10);
	
	// Client Secret Input
	new nanogui::Label(this, "Client Secret:", "sans-bold");
	client_secret_box_ = new nanogui::TextBox(this, "");
	client_secret_box_->set_placeholder("Enter Client Secret");
	client_secret_box_->set_editable(true);
	client_secret_box_->set_password_character('*');
	client_secret_box_->set_fixed_width(350);
	client_secret_box_->set_callback([this](const std::string& value) {
		client_secret_ = value;
		return true;
	});
	
	// Spacer
	new nanogui::Label(this, "", "sans", 20);
	
	// Sync Button
	auto sync_button = new nanogui::Button(this, "Sync");
	sync_button->set_fixed_size(nanogui::Vector2i(100, 30));
	sync_button->set_callback([this]() {
		this->on_sync();
	});
	
	// Spacer
	new nanogui::Label(this, "", "sans", 10);
	
	// Status Label
	status_label_ = new nanogui::Label(this, "", "sans");
	status_label_->set_fixed_size(nanogui::Vector2i(350, 20));
	status_label_->set_color(nanogui::Color(255, 255, 255, 255)); // White color
	
	//	// Initially hide the window
	//	set_visible(false);
	//	set_modal(true);
	
	// Attempt to load existing credentials and synchronize
	if (load_from_file("powerkey.dat")) {
		// Populate the text boxes with loaded data
		api_base_url_box_->set_value(api_base_url_);
		client_id_box_->set_value(client_id_);
		client_secret_box_->set_value(client_secret_);
		
		// Perform synchronization
		on_sync();
	}
	
}

void DeepMotionSettingsWindow::toggle_visibility() {
	set_visible(!is_visible_);
	set_modal(!is_visible_);
	is_visible_ = !is_visible_;
}

void DeepMotionSettingsWindow::on_sync() {
	// Retrieve input values
	std::string api_base_url = api_base_url_box_->value();
	int api_base_port = 443;
	
	// Inside the callback
	std::regex url_regex(R"(^(?:https?://)?([^:/\s]+)(?::(\d+))?)");
	std::smatch matches;
	
	if (std::regex_match(api_base_url, matches, url_regex)) {
		api_base_url = matches[1].str();
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
	} else {
		// Handle invalid URL format
		api_base_url = "";
		api_base_port = 443;
	}
	
	std::string client_id = client_id_box_->value();
	std::string client_secret = client_secret_box_->value();
	
	// Validate input
	if (api_base_url.empty() || client_id.empty() || client_secret.empty()) {
		status_label_->set_caption("API Base URL, Client ID, and Secret cannot be empty.");
		status_label_->set_color(nanogui::Color(255, 0, 0, 255)); // Red color
		return;
	}
	
	// Encode clientId and clientSecret in Base64
	std::string credentials = client_id + ":" + client_secret;
	std::string encoded_credentials = base64_encode(reinterpret_cast<const unsigned char*>(credentials.c_str()), credentials.length());
	
	// Initialize or update the HTTP client with the new API base URL
	_client = std::make_unique<httplib::SSLClient>(api_base_url.c_str(), api_base_port);
	
	// Set Authorization header
	_client->set_default_headers({
		{ "Authorization", "Basic " + encoded_credentials }
	});
	
	// Perform authentication request
	auto res = _client->Get("/account/v1/auth"); // Replace with your auth endpoint
	
	if (res && res->status == 200) {
		auto it = res->headers.find("Set-Cookie");
		if (it != res->headers.end()) {
			_session_cookie = it->second;
			
			// Extract the 'dmsess' cookie value
			std::size_t start_pos = _session_cookie.find("dmsess=");
			if (start_pos != std::string::npos) {
				std::size_t end_pos = _session_cookie.find(";", start_pos);
				if (end_pos != std::string::npos) {
					_session_cookie = _session_cookie.substr(start_pos, end_pos - start_pos);
				} else {
					_session_cookie = _session_cookie.substr(start_pos);
				}
			}
			
			// Set cookie header for future requests if needed
			_client->set_default_headers({
				{ "cookie", _session_cookie }
			});
			
			// Update status label to success
			status_label_->set_caption("Synchronization successful.");
			status_label_->set_color(nanogui::Color(0, 255, 0, 255)); // Green color
			
			// Save credentials and API Base URL if not already saved
			if (!data_saved_) {
				save_to_file("powerkey.dat", api_base_url, std::to_string(api_base_port), client_id, client_secret);
				data_saved_ = true;
			}
			
		} else {
			// Set-Cookie header missing
			status_label_->set_caption("Set-Cookie header missing.");
			status_label_->set_color(nanogui::Color(255, 0, 0, 255)); // Red color
		}
	} else {
		// Authentication failed
		status_label_->set_caption("Invalid credentials or server error.");
		status_label_->set_color(nanogui::Color(255, 0, 0, 255)); // Red color
		data_saved_ = false;
	}
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
			api_base_port_ = std::stoi(value);
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
		status_label_->set_caption("Failed to save credentials.");
		status_label_->set_color(nanogui::Color(255, 0, 0, 255)); // Red color
	}
}
