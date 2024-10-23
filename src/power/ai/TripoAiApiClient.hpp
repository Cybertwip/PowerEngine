#pragma once

#include <httplib.h>
#include <string>
#include <memory>
#include <functional>
#include <mutex>
#include <json/json.h>
#include <sstream>

class TripoAiApiClient {
public:
	// Type aliases for callback functions
	using GenerateModelCallback = std::function<void(const std::string& task_id, const std::string& error_message)>;
	using ConvertModelCallback = std::function<void(const std::string& convert_task_id, const std::string& error_message)>;
	using StatusCallback = std::function<void(const Json::Value& status, const std::string& error_message)>;
	using AuthCallback = std::function<void(bool success, const std::string& error_message)>;
	using DownloadModelCallback = std::function<void(std::stringstream model_stream, const std::string& error_message)>;
	
	/**
	 * @brief Constructs the TripoAiApiClient.
	 */
	TripoAiApiClient();
	
	/**
	 * @brief Destructor for TripoAiApiClient.
	 */
	~TripoAiApiClient();
	
	// Authentication Methods
	
	/**
	 * @brief Authenticates the client with the provided API key by checking the user balance.
	 *
	 * @param api_key Your Tripo3D API key.
	 * @return true If authentication is successful.
	 * @return false If authentication fails.
	 */
	bool authenticate(const std::string& api_key);
	
	/**
	 * @brief Asynchronously authenticates the client with the provided API key by checking the user balance.
	 *
	 * @param api_key Your Tripo3D API key.
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
	
	// Model Generation Methods
	
	/**
	 * @brief Generates a 3D model based on a text prompt.
	 *
	 * @param prompt The text prompt describing the desired model.
	 * @return std::string The Task ID of the model generation request if successful, empty string otherwise.
	 */
	std::string generate_model(const std::string& prompt);
	
	/**
	 * @brief Asynchronously generates a 3D model based on a text prompt.
	 *
	 * @param prompt The text prompt describing the desired model.
	 * @param callback The callback function to be invoked upon completion.
	 */
	void generate_model_async(const std::string& prompt, GenerateModelCallback callback);
	
	// Model Conversion Methods
	
	/**
	 * @brief Converts a generated 3D model to a specified format.
	 *
	 * @param original_task_id The Task ID of the original model generation task.
	 * @param format The desired format for conversion (e.g., "FBX").
	 * @param quad Whether to generate a quad mesh.
	 * @param face_limit The maximum number of faces allowed in the converted model.
	 * @return std::string The Task ID of the model conversion request if successful, empty string otherwise.
	 */
	std::string convert_model(const std::string& original_task_id, const std::string& format, bool quad, int face_limit);
	
	/**
	 * @brief Asynchronously converts a generated 3D model to a specified format.
	 *
	 * @param original_task_id The Task ID of the original model generation task.
	 * @param format The desired format for conversion (e.g., "FBX").
	 * @param quad Whether to generate a quad mesh.
	 * @param face_limit The maximum number of faces allowed in the converted model.
	 * @param callback The callback function to be invoked upon completion.
	 */
	void convert_model_async(const std::string& original_task_id, const std::string& format, bool quad, int face_limit, ConvertModelCallback callback);
	
	// Combined Mesh Generation Methods
	
	/**
	 * @brief Generates a 3D model from a text prompt, polls its status until completion, converts it, and downloads the model.
	 *
	 * @param prompt The text prompt describing the desired model.
	 * @param format The desired format for conversion (e.g., "FBX").
	 * @param quad Whether to generate a quad mesh.
	 * @param face_limit The maximum number of faces allowed in the converted model.
	 * @param callback The callback function to be invoked upon completion with the downloaded model data or an error message.
	 */
	void generate_mesh(const std::string& prompt, const std::string& format, bool quad, int face_limit, DownloadModelCallback callback);
	
	/**
	 * @brief Asynchronously generates a 3D model from a text prompt, polls its status until completion, converts it, and downloads the model.
	 *
	 * @param prompt The text prompt describing the desired model.
	 * @param format The desired format for conversion (e.g., "FBX").
	 * @param quad Whether to generate a quad mesh.
	 * @param face_limit The maximum number of faces allowed in the converted model.
	 * @param callback The callback function to be invoked upon completion with the downloaded model data or an error message.
	 */
	void generate_mesh_async(const std::string& prompt, const std::string& format, bool quad, int face_limit, DownloadModelCallback callback);
	
	// Job Status Methods
	
	/**
	 * @brief Checks the status of a submitted task.
	 *
	 * @param task_id The Task ID of the request.
	 * @return Json::Value The JSON response containing the task status and details.
	 */
	Json::Value check_job_status(const std::string& task_id);
	
	/**
	 * @brief Asynchronously checks the status of a submitted task.
	 *
	 * @param task_id The Task ID of the request.
	 * @param callback The callback function to be invoked with the task status.
	 */
	void check_job_status_async(const std::string& task_id, StatusCallback callback);
	
	/**
	 * @brief Checks the user's balance.
	 *
	 * @return Json::Value The JSON response containing the balance information.
	 */
	Json::Value check_user_balance();
	
private:
	/**
	 * @brief Initializes the HTTP client with default headers.
	 *
	 * @param api_key The API key to be used for authentication.
	 */
	void initialize_client(const std::string& api_key_);
	
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
	
	/**
	 * @brief Converts a string to uppercase.
	 *
	 * @param input The input string.
	 * @return std::string The uppercase version of the input string.
	 */
	std::string to_uppercase(const std::string& input);
	
	// Member variables
	std::string api_key_;                           /**< Tripo3D API key */
	std::unique_ptr<httplib::SSLClient> client_;    /**< HTTP client for API communication */
	mutable std::mutex client_mutex_;               /**< Mutex for thread-safe operations */
};
