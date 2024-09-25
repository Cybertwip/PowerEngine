#pragma once

#include <nanogui/window.h>
#include <nanogui/widget.h>
#include <nanogui/textbox.h>
#include <nanogui/label.h>
#include <nanogui/button.h>

#include <string>
#include <vector>

// Forward declaration of httplib::Client to reduce compilation dependencies
namespace httplib {
class Client;
}

class DeepMotionSettingsWindow : public nanogui::Window {
public:
	/**
	 * @brief Constructs the DeepMotionSettingsWindow.
	 *
	 * @param parent Pointer to the parent widget.
	 * @param api_base_url The base URL for the API endpoints.
	 */
	DeepMotionSettingsWindow(nanogui::Widget* parent);
	
	/**
	 * @brief Toggles the visibility of the settings window.
	 */
	void toggle_visibility();
	
	std::string session_cookie() const {
		return _session_cookie;
	}
	
private:
	// UI Components
	nanogui::TextBox* api_base_url_box_;
	nanogui::TextBox* client_id_box_;
	nanogui::TextBox* client_secret_box_;
	nanogui::Label* status_label_;
	nanogui::Button* close_button_;
	
	// State Variables
	bool is_visible_;
	
	// HTTP Client
	std::unique_ptr<httplib::Client> _client;
	std::string _session_cookie;
	bool data_saved_;
	
	/**
	 * @brief Handles the synchronization process when the "Sync" button is clicked.
	 */
	void on_sync();
	
	/**
	 * @brief Loads credentials from the specified file.
	 *
	 * @param filename The name of the file to load the credentials from.
	 * @return true If credentials were successfully loaded.
	 * @return false If the file does not exist or loading failed.
	 */
	bool load_from_file(const std::string& filename);

	/**
	 * @brief Saves the client credentials to a file.
	 *
	 * @param filename The name of the file to save the credentials.
	 * @param client_id The Client ID to save.
	 * @param client_secret The Client Secret to save.
	 */
	void save_to_file(const std::string& filename, const std::string& api_base_url, const std::string& client_id, const std::string& client_secret);
	
	/**
	 * @brief Represents a DeepMotion model.
	 */
	struct DeepmotionModel {
		std::string id;
		std::string name;
		std::string glb;
		std::string mtime;
	};
	
	// Collection of DeepMotion models (unused in this simplified version)
	std::vector<DeepmotionModel> _deepmotion_models;
	
	// Placeholder for the last selected DeepMotion model (unused in this simplified version)
	std::string last_deepmotion_model_;
	
	std::string api_base_url_;
	std::string client_id_;
	std::string client_secret_;
};
