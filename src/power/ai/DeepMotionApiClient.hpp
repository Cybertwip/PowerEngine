#pragma once

#include <httplib.h>
#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <mutex>
#include <sstream>
#include <json/json.h> // Ensure you have a JSON library like jsoncpp



/**
 * @brief Struct to hold DeepMotion authentication settings.
 */
struct DeepMotionSettings {
	std::string api_base_url;    /**< The base URL of the DeepMotion API. */
	int api_base_port;           /**< The port number of the DeepMotion API. */
	std::string client_id;       /**< The client ID for DeepMotion authentication. */
	std::string client_secret;   /**< The client secret for DeepMotion authentication. */
};

/**
 * @brief A client for interacting with the DeepMotion API.
 *
 * This class provides methods to authenticate, process text to motion, manage models,
 * handle job statuses, and animate models both synchronously and asynchronously using callbacks.
 */
class DeepMotionApiClient {
public:
	// Type aliases for callback functions
	using AuthCallback = std::function<void(bool success, const std::string& error_message)>;
	using ProcessMotionCallback = std::function<void(const std::string& request_id, const std::string& error_message)>;
	using UploadModelCallback = std::function<void(const std::string& model_id, const std::string& error_message)>;
	using StatusCallback = std::function<void(const Json::Value& status, const std::string& error_message)>;
	using DownloadCallback = std::function<void(const Json::Value& results, const std::string& error_message)>;
	using ModelsListCallback = std::function<void(const Json::Value& models, const std::string& error_message)>;
	using AnimateModelCallback = std::function<void(const Json::Value& animation_data, const std::string& error_message)>;
	
	/**
	 * @brief Constructs the DeepMotionApiClient.
	 */
	DeepMotionApiClient();
	
	/**
	 * @brief Destructor for DeepMotionApiClient.
	 */
	~DeepMotionApiClient();
	
	// Asynchronous Methods with Callbacks
	
	/**
	 * @brief Asynchronously authenticates with the DeepMotion API using provided credentials.
	 *
	 * @param api_base_url The base URL of the DeepMotion API.
	 * @param api_base_port The port of the DeepMotion API.
	 * @param client_id The Client ID for authentication.
	 * @param client_secret The Client Secret for authentication.
	 * @param callback The callback function to be invoked upon completion.
	 */
	void authenticate_async(const std::string& api_base_url, int api_base_port,
							const std::string& client_id, const std::string& client_secret,
							AuthCallback callback);
	
	/**
	 * @brief Asynchronously processes text to motion.
	 *
	 * @param prompt The text prompt.
	 * @param model_id The model ID to use.
	 * @param callback The callback function to be invoked upon completion.
	 */
	void process_text_to_motion_async(const std::string& prompt, const std::string& model_id,
									  ProcessMotionCallback callback);
	
	/**
	 * @brief Asynchronously uploads a model file to DeepMotion API.
	 *
	 * @param model_stream The stringstream containing the model data.
	 * @param model_name The name of the model.
	 * @param model_ext The model file extension (e.g., "fbx").
	 * @param callback The callback function to be invoked upon completion.
	 */
	void upload_model_async(std::stringstream model_stream, const std::string& model_name,
							const std::string& model_ext, UploadModelCallback callback);
	
	/**
	 * @brief Asynchronously checks the status of a job.
	 *
	 * @param request_id The ID of the job request.
	 * @param callback The callback function to be invoked upon completion.
	 */
	void check_job_status_async(const std::string& request_id, StatusCallback callback);
	
	/**
	 * @brief Asynchronously downloads the results of a job.
	 *
	 * @param request_id The ID of the job request.
	 * @param callback The callback function to be invoked upon completion.
	 */
	void download_job_results_async(const std::string& request_id, DownloadCallback callback);
	
	/**
	 * @brief Asynchronously lists all available models.
	 *
	 * @param callback The callback function to be invoked upon completion.
	 */
	void list_models_async(ModelsListCallback callback);
	
	/**
	 * @brief Asynchronously animates a model based on a text prompt.
	 *
	 * This method handles the entire workflow:
	 * 1. Processes the text to motion.
	 * 2. Polls the job status until completion.
	 * 3. Downloads the animation results.
	 *
	 * @param prompt The text prompt for animation.
	 * @param model_id The model ID to use for animation.
	 * @param callback The callback function to receive the final animation data or error.
	 */
	void animate_model_async(const std::string& prompt, const std::string& model_id,
							 AnimateModelCallback callback);
	
	// Synchronous Methods (optional to retain for synchronous use)
	
	/**
	 * @brief Authenticates with the DeepMotion API using provided credentials.
	 *
	 * @param api_base_url The base URL of the DeepMotion API.
	 * @param api_base_port The port of the DeepMotion API.
	 * @param client_id The Client ID for authentication.
	 * @param client_secret The Client Secret for authentication.
	 * @return true If authentication was successful.
	 * @return false If authentication failed.
	 */
	bool authenticate(const std::string& api_base_url, int api_base_port,
					  const std::string& client_id, const std::string& client_secret);
	
	/**
	 * @brief Processes text to motion.
	 *
	 * @param prompt The text prompt.
	 * @param model_id The model ID to use.
	 * @return std::string The request ID if successful, empty string otherwise.
	 */
	std::string process_text_to_motion(const std::string& prompt, const std::string& model_id);
	
	/**
	 * @brief Uploads a model file to DeepMotion API.
	 *
	 * @param model_stream The stringstream containing the model data.
	 * @param model_name The name of the model.
	 * @param model_ext The model file extension (e.g., "fbx").
	 * @return std::string The model ID if successful, empty string otherwise.
	 */
	std::string upload_model(std::stringstream& model_stream, const std::string& model_name, const std::string& model_ext);
	
	/**
	 * @brief Checks the status of a job.
	 *
	 * @param request_id The ID of the job request.
	 * @return Json::Value The JSON response containing job status.
	 */
	Json::Value check_job_status(const std::string& request_id);
	
	/**
	 * @brief Downloads the results of a job.
	 *
	 * @param request_id The ID of the job request.
	 * @return Json::Value The JSON response containing download URLs.
	 */
	Json::Value download_job_results(const std::string& request_id);
	
	/**
	 * @brief Retrieves the model upload URL.
	 *
	 * @param resumable Whether the upload is resumable.
	 * @param model_ext The model file extension (e.g., "fbx").
	 * @return std::string The model upload URL if successful, empty string otherwise.
	 */
	std::string get_model_upload_url(bool resumable, const std::string& model_ext);
	
	/**
	 * @brief Stores a model.
	 *
	 * @param model_url The URL where the model has been uploaded.
	 * @param model_name The name of the model.
	 * @return std::string The model ID if successful, empty string otherwise.
	 */
	std::string store_model(const std::string& model_url, const std::string& model_name);
	
	/**
	 * @brief Lists all available models.
	 *
	 * @return Json::Value The JSON response containing the list of models.
	 */
	Json::Value list_models();
	
	/**
	 * @brief Retrieves the session cookie after successful authentication.
	 *
	 * @return std::string The session cookie.
	 */
	std::string get_session_cookie() const;
	
	/**
	 * @brief Checks if the client is authenticated.
	 *
	 * @return true If authenticated.
	 * @return false If not authenticated.
	 */
	bool is_authenticated() const;
	
private:
	/**
	 * @brief Encodes a string to Base64.
	 *
	 * @param input The string to encode.
	 * @return std::string The Base64 encoded string.
	 */
	std::string base64_encode(const std::string& input) const;
	
	/**
	 * @brief Reads a file's contents into a vector of bytes.
	 *
	 * @param file_path The path to the file.
	 * @param data The vector to store the file data.
	 * @return true If the file was read successfully.
	 * @return false If the file could not be read.
	 */
	bool read_file(const std::string& file_path, std::vector<char>& data) const;
	
	/**
	 * @brief Stores the model information using the API.
	 *
	 * @param model_url The URL where the model has been uploaded.
	 * @param model_name The name of the model.
	 * @return std::string The model ID if successful, empty string otherwise.
	 */
	std::string store_model_internal(const std::string& model_url, const std::string& model_name);
	
	/**
	 * @brief Helper function to convert a string to uppercase.
	 *
	 * @param input The input string.
	 * @return std::string The uppercase version of the input string.
	 */
	std::string to_uppercase(const std::string& input) const;
	
	// Member variables
	std::unique_ptr<httplib::SSLClient> client_; /**< HTTP client for API communication */
	std::string session_cookie_;                  /**< Session cookie after authentication */
	bool authenticated_;                          /**< Authentication status */
	mutable std::mutex client_mutex_;             /**< Mutex for thread-safe operations */
};
