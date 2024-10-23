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

class PowerAi;


class PowerSettingsWindow : public nanogui::Window {
public:
	/**
	 * @brief Constructs the PowerSettingsWindow.
	 *
	 * @param parent The parent nanogui screen.
	 * @param powerAi Reference to the PowerAi instance for authentication.
	 * @param successCallback Callback to be invoked upon successful authentication.
	 */
	PowerSettingsWindow(nanogui::Screen& parent, PowerAi& powerAi, std::function<void()> successCallback);
	
	~PowerSettingsWindow() = default;
	
private:
	/**
	 * @brief Handles the synchronization/authentication process.
	 */
	void on_sync();
	
	/**
	 * @brief Loads credentials from a file.
	 *
	 * @param filename The file from which to load credentials.
	 * @return true If loading is successful.
	 * @return false Otherwise.
	 */
	bool load_from_file(const std::string& filename);
	
	/**
	 * @brief Saves credentials to a file.
	 *
	 * @param filename The file to which credentials will be saved.
	 * @param api_key The OpenAI API key.
	 * @param tripo_ai_key The Tripo AI API key.
	 * @param deepmotion_api_base_url The DeepMotion API base URL.
	 * @param deepmotion_api_base_port The DeepMotion API base port.
	 * @param deepmotion_client_id The DeepMotion Client ID.
	 * @param deepmotion_client_secret The DeepMotion Client Secret.
	 */
	bool save_to_file(const std::string& filename,
					  const std::string& api_key,
					  const std::string& tripo_ai_key,
					  const std::string& deepmotion_api_base_url,
					  const std::string& deepmotion_api_base_port,
					  const std::string& deepmotion_client_id,
					  const std::string& deepmotion_client_secret);
	
	// UI Components
	std::shared_ptr<nanogui::Label> m_api_key_label_;
	std::shared_ptr<nanogui::TextBox> api_key_box_;
	
	std::shared_ptr<nanogui::Label> m_tripo_ai_key_label_;
	std::shared_ptr<nanogui::TextBox> tripo_ai_key_box_;
	
	std::shared_ptr<nanogui::Label> m_deepmotion_url_label_;
	std::shared_ptr<nanogui::TextBox> deepmotion_url_box_;
	
	std::shared_ptr<nanogui::Label> m_deepmotion_port_label_;
	std::shared_ptr<nanogui::TextBox> deepmotion_port_box_;
	
	std::shared_ptr<nanogui::Label> m_deepmotion_client_id_label_;
	std::shared_ptr<nanogui::TextBox> deepmotion_client_id_box_;
	
	std::shared_ptr<nanogui::Label> m_deepmotion_client_secret_label_;
	std::shared_ptr<nanogui::TextBox> deepmotion_client_secret_box_;
	
	std::shared_ptr<nanogui::Label> status_label_;
	
	std::shared_ptr<nanogui::Button> sync_button_;
	std::shared_ptr<nanogui::Button> close_button_;
	
	std::shared_ptr<nanogui::Widget> m_button_panel_;
	
	// Internal State
	bool data_saved_;
	std::function<void()> mSuccessCallback;
	PowerAi& mPowerAi;
	
	// Stored credentials
	std::string api_key_;
	std::string tripo_ai_key_;
	std::string deepmotion_api_base_url_;
	std::string deepmotion_api_base_port_;
	std::string deepmotion_client_id_;
	std::string deepmotion_client_secret_;
};
