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
	 * @brief Struct to hold the final animation callback.
	 */
	using GenerateAnimationCallback = std::function<void(std::stringstream fbx_stream, const std::string& error_message)>;
	
	// Existing Methods
	DeepMotionApiClient();
	~DeepMotionApiClient();
	
	// Asynchronous Methods with Callbacks
	void authenticate_async(const std::string& api_base_url, int api_base_port,
							const std::string& client_id, const std::string& client_secret,
							AuthCallback callback);
	
	void process_text_to_motion_async(const std::string& prompt, const std::string& model_id,
									  ProcessMotionCallback callback);
	
	void upload_model_async(std::stringstream model_stream, const std::string& model_name,
							const std::string& model_ext, UploadModelCallback callback);
	
	void check_job_status_async(const std::string& request_id, StatusCallback callback);
	
	void download_job_results_async(const std::string& request_id, DownloadCallback callback);
	
	void list_models_async(ModelsListCallback callback);
	
	void animate_model_async(const std::string& prompt, const std::string& model_id,
							 AnimateModelCallback callback);
	
	// New Asynchronous Method
	/**
	 * @brief Asynchronously generates an animation by uploading a model, processing a text prompt,
	 *        polling the job status, and retrieving the resulting FBX file.
	 *
	 * @param model_stream The stringstream containing the model data.
	 * @param model_name The name of the model.
	 * @param model_ext The model file extension (e.g., "fbx").
	 * @param prompt The text prompt for animation generation.
	 * @param callback The callback function to receive the FBX stream or an error message.
	 */
	void generate_animation_async(std::stringstream model_stream, const std::string& model_name, const std::string& model_ext,
								  const std::string& prompt, GenerateAnimationCallback callback);
	
	// Synchronous Methods (optional to retain for synchronous use)
	bool authenticate(const std::string& api_base_url, int api_base_port,
					  const std::string& client_id, const std::string& client_secret);
	
	std::string process_text_to_motion(const std::string& prompt, const std::string& model_id);
	
	std::string upload_model(std::stringstream& model_stream, const std::string& model_name, const std::string& model_ext);
	
	Json::Value check_job_status(const std::string& request_id);
	
	Json::Value download_job_results(const std::string& request_id);
	
	std::string get_model_upload_url(bool resumable, const std::string& model_ext);
	
	std::string store_model(const std::string& model_url, const std::string& model_name);
	
	Json::Value list_models();
	
	std::string get_session_cookie() const;
	
	bool is_authenticated() const;
	
private:
	// Private Helper Methods
	std::string base64_encode(const std::string& input) const;
	bool read_file(const std::string& file_path, std::vector<char>& data) const;
	std::string store_model_internal(const std::string& model_url, const std::string& model_name);
	std::string to_uppercase(const std::string& input) const;
	
	// Member variables
	std::unique_ptr<httplib::SSLClient> client_; /**< HTTP client for API communication */
	std::string session_cookie_;                  /**< Session cookie after authentication */
	bool authenticated_;                          /**< Authentication status */
	mutable std::mutex client_mutex_;             /**< Mutex for thread-safe operations */
};
