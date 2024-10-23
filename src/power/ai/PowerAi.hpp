#pragma once

#include <functional>
#include <string>

#include <iostream>
#include <json/json.h>


class DeepMotionApiClient;
class OpenAiApiClient;
class TripoAiApiClient;


/**
 * @brief A high-level AI manager that integrates OpenAI, Tripo3D, and DeepMotion APIs.
 *
 * The PowerAi class provides methods to generate images, meshes, and 3D animations
 * asynchronously by leveraging the underlying API clients.
 */
class PowerAi {
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
	 * @brief Asynchronously generates an image based on a text prompt.
	 *
	 * @param prompt The text prompt describing the desired image.
	 * @param callback The callback function to receive the image URL or an error message.
	 *                 Signature: void(const std::string& image_url, const std::string& error_message)
	 */
	void generate_image_async(const std::string& prompt,
							  std::function<void(const std::string& image_url, const std::string& error_message)> callback);
	
	/**
	 * @brief Asynchronously generates a 3D mesh based on a text prompt.
	 *
	 * @param prompt The text prompt describing the desired 3D mesh.
	 * @param callback The callback function to receive the mesh task ID or an error message.
	 *                 Signature: void(const std::string& task_id, const std::string& error_message)
	 */
	void generate_mesh_async(const std::string& prompt,
							 std::function<void(const std::string& task_id, const std::string& error_message)> callback);
	
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
	
private:
	OpenAiApiClient& mOpenAiApiClient;          /**< Reference to OpenAiApiClient */
	TripoAiApiClient& mTripoAiApiClient;        /**< Reference to TripoAiApiClient */
	DeepMotionApiClient& mDeepMotionApiClient;  /**< Reference to DeepMotionApiClient */
};
