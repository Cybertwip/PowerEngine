#pragma once

#include <httplib.h>
#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <map>
#include <json/json.h> // Ensure you have a JSON library like jsoncpp

class DeepMotionApiClient {
public:
	/**
	 * @brief Constructs the DeepMotionApiClient.
	 */
	DeepMotionApiClient();
	
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
	 * @return bool True if the model was stored successfully.
	 */
	bool store_model(const std::string& model_url, const std::string& model_name);
	
	/**
	 * @brief Lists all available models.
	 *
	 * @return Json::Value The JSON response containing the list of models.
	 */
	Json::Value list_models();
	
	/**
	 * @brief Uploads a model file to DeepMotion API.
	 *
	 * This method performs the following steps:
	 * 1. Extracts the model name and extension from the provided file path.
	 * 2. Retrieves an upload URL from the API.
	 * 3. Uploads the model file to the obtained URL.
	 * 4. Stores the model information using the API.
	 *
	 * @return true If the upload was successful.
	 * @return false If the upload failed.
	 */
	bool upload_model(const std::vector<char>& model_data, const std::string& model_name, const std::string& model_ext);

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
	std::string base64_encode(const std::string& input);
	
	/**
	 * @brief Reads a file's contents into a vector of bytes.
	 *
	 * @param file_path The path to the file.
	 * @param data The vector to store the file data.
	 * @return true If the file was read successfully.
	 * @return false If the file could not be read.
	 */
	bool read_file(const std::string& file_path, std::vector<char>& data);
	
	std::unique_ptr<httplib::SSLClient> client_;
	std::string session_cookie_;
	bool authenticated_;
};
