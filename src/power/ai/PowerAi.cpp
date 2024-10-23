#include "PowerAi.hpp"

#include <iostream>
#include <json/json.h>
#include <mutex>
#include <memory>

/**
 * @brief Constructs the PowerAi instance with references to the API clients.
 */
PowerAi::PowerAi(OpenAiApiClient& openAiApiClient,
				 TripoAiApiClient& tripoAiApiClient,
				 DeepMotionApiClient& deepMotionApiClient)
: mOpenAiApiClient(openAiApiClient),
mTripoAiApiClient(tripoAiApiClient),
mDeepMotionApiClient(deepMotionApiClient) {
	// Constructor body (if needed)
}

/**
 * @brief Destructor for PowerAi.
 */
PowerAi::~PowerAi() {
	// Destructor body (if needed)
}

/**
 * @brief Asynchronously authenticates with OpenAI, Tripo AI, and DeepMotion using provided credentials.
 */
void PowerAi::authenticate_async(const std::string& openai_api_key,
								 const std::string& tripo_ai_api_key,
								 const std::string& deepmotion_api_base_url,
								 int deepmotion_api_base_port,
								 const std::string& deepmotion_client_id,
								 const std::string& deepmotion_client_secret,
								 std::function<void(bool success, const std::string& error_message)> callback) {
	
	mAuthState = std::make_shared<AuthState>();
	
	// Lambda to check if all authentications are completed
	auto check_and_callback = [this, callback]() {
		if (mAuthState->completed == 3) { // Total number of services
			callback(mAuthState->success, mAuthState->error_message);
		}
	};
	
	// Authenticate OpenAI
	mOpenAiApiClient.authenticate_async(openai_api_key, [this, check_and_callback](bool success, const std::string& error_message) {
		std::lock_guard<std::mutex> lock(mAuthState->mutex);
		if (!success) {
			mAuthState->success = false;
			mAuthState->error_message += "OpenAI Authentication Failed: " + error_message + "\n";
		}
		mAuthState->completed++;
		check_and_callback();
	});
	
	// Authenticate Tripo AI
	mTripoAiApiClient.authenticate_async(tripo_ai_api_key, [this, check_and_callback](bool success, const std::string& error_message) {
		std::lock_guard<std::mutex> lock(mAuthState->mutex);
		if (!success) {
			mAuthState->success = false;
			mAuthState->error_message += "Tripo AI Authentication Failed: " + error_message + "\n";
		}
		mAuthState->completed++;
		check_and_callback();
	});
	
	// Authenticate DeepMotion
	mDeepMotionApiClient.authenticate_async(deepmotion_api_base_url, deepmotion_api_base_port, deepmotion_client_id, deepmotion_client_secret,
											[this, check_and_callback](bool success, const std::string& error_message) {
		std::lock_guard<std::mutex> lock(mAuthState->mutex);
		if (!success) {
			mAuthState->success = false;
			mAuthState->error_message += "DeepMotion Authentication Failed: " + error_message + "\n";
		}
		mAuthState->completed++;
		check_and_callback();
	});
}

/**
 * @brief Asynchronously generates an image based on a text prompt.
 */
void PowerAi::generate_image_async(const std::string& prompt,
								   std::function<void(const std::string& image_url, const std::string& error_message)> callback) {
	// Delegate to OpenAiApiClient
	mOpenAiApiClient.generate_image_async(prompt, [callback](const std::string& image_url, const std::string& error_message) {
		if (error_message.empty()) {
			std::cout << "Image generated successfully. URL: " << image_url << std::endl;
			callback(image_url, "");
		} else {
			std::cerr << "Failed to generate image: " << error_message << std::endl;
			callback("", error_message);
		}
	});
}

/**
 * @brief Asynchronously generates a 3D mesh based on a text prompt.
 */
void PowerAi::generate_mesh_async(const std::string& prompt,
								  std::function<void((std::stringstream model_stream, const std::string& error))> callback) {
	// Delegate to TripoAiApiClient
	mTripoAiApiClient.generate_mesh_async(prompt, [callback](std::stringstream model_stream, const std::string& error) {
		if (error_message.empty()) {
			std::cout << "Mesh generation task submitted successfully. Task ID: " << task_id << std::endl;
			callback(model_stream, "");
		} else {
			std::cerr << "Failed to generate mesh: " << error_message << std::endl;
			callback({}, error_message);
		}
	});
}

/**
 * @brief Asynchronously generates a 3D animation based on a text prompt and a model ID.
 */
void PowerAi::generate_3d_animation_async(const std::string& prompt, const std::string& model_id,
										  std::function<void(const Json::Value& animation_data, const std::string& error_message)> callback) {
	// Delegate to DeepMotionApiClient
	mDeepMotionApiClient.animate_model_async(prompt, model_id, [callback](const Json::Value& animation_data, const std::string& error_message) {
		if (error_message.empty()) {
			std::cout << "3D Animation generated successfully." << std::endl;
			callback(animation_data, "");
		} else {
			std::cerr << "Failed to generate 3D animation: " << error_message << std::endl;
			callback(Json::Value(), error_message);
		}
	});
}

/**
 * @brief Asynchronously generates text based on a text prompt.
 */
void PowerAi::generate_text_async(const std::string& prompt,
								  std::function<void(const std::string& text, const std::string& error_message)> callback) {
	// Delegate to OpenAiApiClient
	mOpenAiApiClient.generate_text_async(prompt, [callback](const std::string& text, const std::string& error_message) {
		if (error_message.empty()) {
			std::cout << "Text generated successfully: " << text << std::endl;
			callback(text, "");
		} else {
			std::cerr << "Failed to generate text: " << error_message << std::endl;
			callback("", error_message);
		}
	});
}

/**
 * @brief Asynchronously generates text based on a text prompt and image data.
 */
void PowerAi::generate_text_async(const std::string& prompt, const std::vector<uint8_t>& image_data,
								  std::function<void(const std::string& text, const std::string& error_message)> callback) {
	// Delegate to OpenAiApiClient with image data
	mOpenAiApiClient.generate_text_async(prompt, image_data, [callback](const std::string& text, const std::string& error_message) {
		if (error_message.empty()) {
			std::cout << "Text generated successfully with image data: " << text << std::endl;
			callback(text, "");
		} else {
			std::cerr << "Failed to generate text with image data: " << error_message << std::endl;
			callback("", error_message);
		}
	});
}
