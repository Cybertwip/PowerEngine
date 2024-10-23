#pragma once

#include <nanogui/window.h>
#include <nanogui/widget.h>
#include <nanogui/textbox.h>
#include <nanogui/label.h>
#include <nanogui/button.h>

#include <string>
#include <vector>
#include <functional>
#include <memory>

// Forward declaration of OpenAiApiClient to reduce compilation dependencies
class OpenAiApiClient;

/**
 * @brief A settings window for configuring and authenticating with the OpenAI DALL-E API.
 */
class DallESettingsWindow : public nanogui::Window {
public:
	/**
	 * @brief Constructs the DallESettingsWindow.
	 *
	 * @param parent Reference to the parent nanogui::Screen.
	 * @param dalleClient Reference to an instance of OpenAiApiClient.
	 * @param successCallback Callback function to be invoked upon successful authentication.
	 */
	DallESettingsWindow(nanogui::Screen& parent, OpenAiApiClient& dalleClient, std::function<void()> successCallback);
	
private:
	// UI Components
	std::shared_ptr<nanogui::TextBox> api_key_box_;
	std::shared_ptr<nanogui::Widget> m_button_panel_;
	std::shared_ptr<nanogui::Button> sync_button_;
	std::shared_ptr<nanogui::Button> close_button_;
	std::shared_ptr<nanogui::Label> m_api_key_label_;
	std::shared_ptr<nanogui::Label> status_label_;
	
	// Internal state
	bool data_saved_;
	
	// Callback upon successful authentication
	std::function<void()> mSuccessCallback;
	
	// Reference to the OpenAiApiClient
	OpenAiApiClient& mOpenAiApiClient;
	
	/**
	 * @brief Handles the synchronization (authentication) process when the "Sync" button is clicked.
	 */
	void on_sync();
	
	/**
	 * @brief Loads API key from the specified file.
	 *
	 * @param filename The name of the file to load the API key from.
	 * @return true If the API key was successfully loaded.
	 * @return false If loading failed.
	 */
	bool load_from_file(const std::string& filename);
	
	/**
	 * @brief Saves the API key to a specified file.
	 *
	 * @param filename The name of the file to save the API key to.
	 * @param api_key The API key to save.
	 */
	bool save_to_file(const std::string& filename, const std::string& api_key);
};
