#pragma once

#include <httplib.h>
#include <string>
#include <memory>
#include <functional>
#include <mutex>
#include <json/json.h>
#include <sstream>

class OpenAiApiClient {
public:
	// Type aliases for callback functions
	using GenerateImageCallback = std::function<void(const std::string& image_url, const std::string& error_message)>;
	using DownloadImageCallback = std::function<void(std::stringstream image_stream, const std::string& error_message)>;
	using GenerateTextCallback = std::function<void(const std::string& generated_text, const std::string& error_message)>;
	using ListModelsCallback = std::function<void(const Json::Value& models, const std::string& error_message)>;
	using AuthCallback = std::function<void(bool success, const std::string& error_message)>;
	
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
	 * @brief Authenticates the client with the provided API key by listing models.
	 *
	 * @param api_key Your OpenAI API key.
	 * @return true If authentication is successful.
	 * @return false If authentication fails.
	 */
	bool authenticate(const std::string& api_key);
	
	/**
	 * @brief Asynchronously authenticates the client with the provided API key by listing models.
	 *
	 * @param api_key Your OpenAI API key.
	 * @param callback The callback function to be invoked upon completion.
	 */
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
	
	/**
	 * @brief Retrieves the current API key.
	 *
	 * @return std::string The current API key.
	 */
	std::string get_api_key() const;
	
	// Image Generation Methods
	
	/**
	 * @brief Generates an image based on a text prompt.
	 *
	 * @param prompt The text prompt describing the desired image.
	 * @return std::string The URL of the generated image if successful, empty string otherwise.
	 */
	std::string generate_image(const std::string& prompt);
	
	/**
	 * @brief Asynchronously generates an image based on a text prompt.
	 *
	 * @param prompt The text prompt describing the desired image.
	 * @param callback The callback function to be invoked upon completion with the image URL or an error message.
	 */
	void generate_image_async(const std::string& prompt, GenerateImageCallback callback);
	
	/**
	 * @brief Asynchronously generates an image based on a text prompt, downloads the image data, and passes it via callback.
	 *
	 * @param prompt The text prompt describing the desired image.
	 * @param callback The callback function to be invoked upon completion with the image data as a stringstream or an error message.
	 */
	void generate_image_download_async(const std::string& prompt, DownloadImageCallback callback);
	
	// Text Generation Methods
	
	/**
	 * @brief Generates text based on a prompt.
	 *
	 * @param prompt The text prompt.
	 * @return std::string The generated text if successful, empty string otherwise.
	 */
	std::string generate_text(const std::string& prompt);
	
	/**
	 * @brief Asynchronously generates text based on a prompt.
	 *
	 * @param prompt The text prompt.
	 * @param callback The callback function to be invoked upon completion with the generated text or an error message.
	 */
	void generate_text_async(const std::string& prompt, GenerateTextCallback callback);
	
	// Model Listing Methods
	
	/**
	 * @brief Lists available models.
	 *
	 * @return Json::Value The JSON response containing the list of models if successful, empty JSON otherwise.
	 */
	Json::Value list_models();
	
	/**
	 * @brief Asynchronously lists available models.
	 *
	 * @param callback The callback function to be invoked upon completion with the list of models or an error message.
	 */
	void list_models_async(ListModelsCallback callback);
	
private:
	/**
	 * @brief Initializes the HTTP client with default headers.
	 *
	 * @param api_key The API key to be used for authentication.
	 */
	void initialize_client(const std::string& api_key);
	
	/**
	 * @brief Downloads a file from the given URL and stores it in a stringstream.
	 *
	 * @param url The URL of the file to download.
	 * @param data_stream The stringstream to store the downloaded data.
	 * @return true If the download is successful.
	 * @return false If the download fails.
	 */
	bool download_file(const std::string& url, std::stringstream& data_stream);
	
	/**
	 * @brief Parses a URL into host and path components.
	 *
	 * @param url The URL to parse.
	 * @param host The extracted host component.
	 * @param path The extracted path component.
	 * @return true If the URL is parsed successfully.
	 * @return false If the URL is invalid.
	 */
	bool parse_url(const std::string& url, std::string& host, std::string& path);
	
	// Member variables
	std::string api_key_;                           /**< OpenAI API key */
	std::unique_ptr<httplib::SSLClient> client_;    /**< HTTP client for API communication */
	mutable std::mutex client_mutex_;               /**< Mutex for thread-safe operations */
};
