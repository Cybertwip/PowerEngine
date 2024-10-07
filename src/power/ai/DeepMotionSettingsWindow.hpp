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
class SSLClient;
}

class DeepMotionApiClient;

class DeepMotionSettingsWindow : public nanogui::Window {
public:
	/**
	 * @brief Constructs the DeepMotionSettingsWindow.
	 *
	 * @param parent Pointer to the parent widget.
	 * @param api_base_url The base URL for the API endpoints.
	 */
	DeepMotionSettingsWindow(std::shared_ptr<nanogui::Screen> parent, DeepMotionApiClient& deepMotionApiClient, std::function<void()> successCallback);
	
private:
	void initialize() override;
	
	// UI Components
	std::shared_ptr<nanogui::TextBox> api_base_url_box_;
	std::shared_ptr<nanogui::TextBox> client_id_box_;
	std::shared_ptr<nanogui::TextBox> client_secret_box_;
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
	void save_to_file(const std::string& filename, const std::string& api_base_url, const std::string& api_base_port, const std::string& client_id, const std::string& client_secret);
	
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
	int api_base_port_;
	std::string client_id_;
	std::string client_secret_;
	
	std::function<void()> mSuccessCallback;
	
	DeepMotionApiClient& mDeepMotionApiClient;
	
	std::shared_ptr<nanogui::Button> mSyncButton;
	std::shared_ptr<nanogui::Button> mDeepMotionButton;
	std::shared_ptr<nanogui::Button> mCloseButton;
	std::shared_ptr<nanogui::Label> mStatusLabel;
	std::shared_ptr<nanogui::Label> mApiBaseLabel;
	std::shared_ptr<nanogui::Label> mClientIdLabel;
	std::shared_ptr<nanogui::Label> mClientSecretLabel;
	
	std::shared_ptr<nanogui::Widget> mStatusPanel;
	std::shared_ptr<nanogui::Widget> mTopPanel;
	std::shared_ptr<nanogui::ImageView> mImageView;
};
