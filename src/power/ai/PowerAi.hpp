#pragma once

#include "DeepMotionApiClient.hpp"
#include "OpenAiApiClient.hpp"
#include "TripoAiApiClient.hpp"

#include <functional>
#include <sstream>
#include <string>
#include <json/json.h>

/**
 * @brief A high-level AI manager that integrates OpenAI, Tripo3D, and DeepMotion APIs.
 *
 * The PowerAi class provides methods to generate images, meshes, and 3D animations
 * asynchronously by leveraging the underlying API clients.
 */
class PowerAi {
private:
	// Shared state to track authentication results
	struct AuthState {
		std::mutex mutex;
		int completed = 0;
		bool success = true;
		std::string error_message;
	};

public:
	/**
	 * @brief Constructs the PowerAi instance with references to the API clients.
	 *
	 * @param openAiApiClient Reference to an instance of OpenAiApiClient.
	 * @param tripoAiApiClient Reference to an instance of TripoAiApiClient.
	 * @param deepMotionApiClient Reference to an instance of DeepMotionApiClient.
	 */
	PowerAi(OpenAiApiClient& openAiApiClient,
			TripoAiApiClient& tripoAiApiClient,
			DeepMotionApiClient& deepMotionApiClient);
	
	/**
	 * @brief Destructor for PowerAi.
	 */
	~PowerAi();
	
	// Asynchronous Methods
	
	/**
	 * @brief Asynchronously authenticates with OpenAI, Tripo AI, and DeepMotion using provided credentials.
	 *
	 * @param openai_api_key The API key for OpenAI.
	 * @param tripo_ai_api_key The API key for Tripo AI.
	 * @param deepmotion_api_base_url The base URL for DeepMotion API.
	 * @param deepmotion_api_base_port The base port for DeepMotion API.
	 * @param deepmotion_client_id The Client ID for DeepMotion authentication.
	 * @param deepmotion_client_secret The Client Secret for DeepMotion authentication.
	 * @param callback The callback function to receive the authentication result.
	 *                 Signature: void(bool success, const std::string& error_message)
	 */
	void authenticate_async(const std::string& openai_api_key,
							const std::string& tripo_ai_api_key,
							const std::string& deepmotion_api_base_url,
							int deepmotion_api_base_port,
							const std::string& deepmotion_client_id,
							const std::string& deepmotion_client_secret,
							std::function<void(bool success, const std::string& error_message)> callback);
	
	/**
	 * @brief Asynchronously generates an image based on a text prompt.
	 *
	 * @param prompt The text prompt describing the desired image.
	 * @param callback The callback function to receive the image URL or an error message.
	 *                 Signature: void(const std::string& image_url, const std::string& error_message)
	 */
	void generate_image_async(const std::string& prompt,
							  std::function<void((std::stringstream image_stream, const std::string& error))> callback);
	
	/**
	 * @brief Asynchronously generates a 3D mesh based on a text prompt.
	 *
	 * @param prompt The text prompt describing the desired 3D mesh.
	 * @param callback The callback function to receive the mesh task ID or an error message.
	 *                 Signature: void(const std::string& task_id, const std::string& error_message)
	 */
	void generate_mesh_async(const std::string& prompt,
							 std::function<void((std::stringstream model_stream, const std::string& error))> callback);
	
	/**
	 * @brief Asynchronously generates a 3D animation based on a text prompt and a model ID.
	 *
	 * @param prompt The text prompt describing the desired animation.
	 * @param model_id The model ID to use for animation.
	 * @param callback The callback function to receive the animation data or an error message.
	 *                 Signature: void(const Json::Value& animation_data, const std::string& error_message)
	 */
	void generate_3d_animation_async(const std::string& prompt, const std::string& model_id,
									 std::function<void(const Json::Value& animation_data, const std::string& error_message)> callback);
	

	void generate_text_async(const std::string& prompt,
						std::function<void(const std::string& text, const std::string& error_message)> callback);
	
	void generate_text_async(const std::string& prompt, const std::vector<uint8_t>& image_data,
									  std::function<void(const std::string& text, const std::string& error_message)> callback);
	// Additional Methods and Members as Needed
	
private:
	OpenAiApiClient& mOpenAiApiClient;          /**< Reference to OpenAiApiClient */
	TripoAiApiClient& mTripoAiApiClient;        /**< Reference to TripoAiApiClient */
	DeepMotionApiClient& mDeepMotionApiClient;  /**< Reference to DeepMotionApiClient */
	
	std::shared_ptr<AuthState> mAuthState;
};
