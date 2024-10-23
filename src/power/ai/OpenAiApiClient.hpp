#pragma once

#include <httplib.h>
#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <mutex>
#include <sstream>
#include <json/json.h>

class OpenAiApiClient {
public:
	// Type aliases for callback functions
	using GenerateImageCallback = std::function<void(const std::string& image_url, const std::string& error_message)>;
	using ListModelsCallback = std::function<void(const Json::Value& models, const std::string& error_message)>;
	
	// Type alias for text generation callback
	using GenerateTextCallback = std::function<void(const std::string& text, const std::string& error_message)>;
	
	/**
	 * @brief Constructs the OpenAiApiClient.
	 */
	OpenAiApiClient();
	
	/**
	 * @brief Destructor for OpenAiApiClient.
	 */
	~OpenAiApiClient();
	
	// Authentication Methods
	
	/**
	 * @brief Authenticates the client with the provided API key.
	 *
	 * @param api_key Your OpenAI API key.
	 * @return true If authentication is successful.
	 * @return false If authentication fails.
	 */
	bool authenticate(const std::string& api_key);
	
	/**
	 * @brief Asynchronously authenticates the client with the provided API key.
	 *
	 * @param api_key Your OpenAI API key.
	 * @param callback The callback function to be invoked upon completion.
	 */
	using AuthCallback = std::function<void(bool success, const std::string& error_message)>;
	void authenticate_async(const std::string& api_key, AuthCallback callback);
	
	/**
	 * @brief Saves the API key to a file.
	 *
	 * @param file_path The path to the file where the API key will be saved.
	 * @return true If the API key is saved successfully.
	 * @return false If saving fails.
	 */
	bool save_api_key(const std::string& file_path) const;
	
	/**
	 * @brief Loads the API key from a file.
	 *
	 * @param file_path The path to the file from which the API key will be loaded.
	 * @return true If the API key is loaded successfully.
	 * @return false If loading fails.
	 */
	bool load_api_key(const std::string& file_path);
	
	std::string get_api_key() const {
		return api_key_;
	}
	
	// Asynchronous Methods with Callbacks
	
	/**
	 * @brief Asynchronously generates an image from a text prompt.
	 *
	 * @param prompt The text prompt describing the desired image.
	 * @param callback The callback function to be invoked upon completion.
	 */
	void generate_image_async(const std::string& prompt, GenerateImageCallback callback);
	
	/**
	 * @brief Asynchronously lists all available models.
	 *
	 * @param callback The callback function to be invoked upon completion.
	 */
	void list_models_async(ListModelsCallback callback);
	
	/**
	 * @brief Asynchronously generates text using the vision module by providing a prompt and an image.
	 *
	 * @param prompt The text prompt for the model.
	 * @param image_data The raw PNG image data as a vector of bytes.
	 * @param callback The callback function to be invoked upon completion.
	 */
	void generate_text_async(const std::string& prompt, const std::vector<uint8_t>& image_data, GenerateTextCallback callback);
	
	/**
	 * @brief Asynchronously generates text based solely on a text prompt.
	 *
	 * @param prompt The text prompt for the model.
	 * @param callback The callback function to be invoked upon completion.
	 */
	void generate_text_async(const std::string& prompt, GenerateTextCallback callback);
	
	// Synchronous Methods
	
	/**
	 * @brief Generates an image from a text prompt.
	 *
	 * @param prompt The text prompt describing the desired image.
	 * @return std::string The URL of the generated image if successful, empty string otherwise.
	 */
	std::string generate_image(const std::string& prompt);
	
	/**
	 * @brief Lists all available models.
	 *
	 * @return Json::Value The JSON response containing the list of models.
	 */
	Json::Value list_models();
	
	/**
	 * @brief Generates text using the vision module by providing a prompt and an image.
	 *
	 * @param prompt The text prompt for the model.
	 * @param image_data The raw PNG image data as a vector of bytes.
	 * @return std::string The generated text if successful, empty string otherwise.
	 */
	std::string generate_text(const std::string& prompt, const std::vector<uint8_t>& image_data);
	
	/**
	 * @brief Generates text based solely on a text prompt.
	 *
	 * @param prompt The text prompt for the model.
	 * @return std::string The generated text if successful, empty string otherwise.
	 */
	std::string generate_text(const std::string& prompt);
	
private:
	/**
	 * @brief Initializes the HTTP client with default headers.
	 *
	 * @param api_key The API key to be used for authentication.
	 */
	void initialize_client(const std::string& api_key);
	
	/**
	 * @brief Encodes a string to Base64.
	 *
	 * @param input The string to encode.
	 * @return std::string The Base64 encoded string.
	 */
	std::string base64_encode(const std::string& input) const;
	
	// Member variables
	std::string api_key_;                           /**< OpenAI API key */
	std::unique_ptr<httplib::SSLClient> client_;    /**< HTTP client for API communication */
	mutable std::mutex client_mutex_;               /**< Mutex for thread-safe operations */
};
