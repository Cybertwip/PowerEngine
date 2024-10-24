#include "PowerAi.hpp"

#include <iostream>
#include <json/json.h>
#include <mutex>
#include <memory>

namespace PowerAiTools {
std::stringstream duplicateStringStream(std::stringstream& original) {
	// Create a new stringstream
	std::stringstream copy;
	
	// Copy the string content
	copy.str(original.str());
	
	// Copy formatting flags
	copy.copyfmt(original);
	
	// Copy the state flags
	copy.clear(original.rdstate());
	
	// Copy the get (read) position
	std::streampos original_get_pos = original.tellg();
	if (original_get_pos != -1) { // Check if tellg() succeeded
		copy.seekg(original_get_pos);
	}
	
	// Copy the put (write) position
	std::streampos original_put_pos = original.tellp();
	if (original_put_pos != -1) { // Check if tellp() succeeded
		copy.seekp(original_put_pos);
	}
	
	return copy;
}
}

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
								   std::function<void((std::stringstream image_stream, const std::string& error))> callback) {
	// Delegate to OpenAiApiClient
	mOpenAiApiClient.generate_image_download_async(prompt, [callback](std::stringstream image_stream, const std::string& error_message) {
		if (error_message.empty()) {
			std::cout << "Image generated successfully" << std::endl;
			callback(std::move(image_stream), "");
		} else {
			std::cerr << "Failed to generate image: " << error_message << std::endl;
			callback({}, error_message);
		}
	});
}

/**
 * @brief Asynchronously generates a 3D mesh based on a text prompt.
 */
void PowerAi::generate_mesh_async(const std::string& prompt,
								  const std::string& animation_prompt,
								  bool generate_rig,
								  bool generate_animation,
								  std::function<void(std::stringstream, const std::string&)> callback) {
	
	struct StreamContainer {
		std::stringstream stream;
	};
	
	auto container = std::make_shared<StreamContainer>();
		
	// Delegate to TripoAiApiClient
	mTripoAiApiClient.generate_mesh_async(prompt, "fbx", false, 10000, generate_rig, [this, prompt, animation_prompt, callback, generate_rig, generate_animation, container](std::stringstream model_stream, const std::string& error_message) {
		if (error_message.empty()) {
			std::cout << "Mesh generation successful" << std::endl;
			
			std::stringstream original_model_stream = PowerAiTools::duplicateStringStream(model_stream);

			container->stream = std::move(model_stream);

//			if (generate_rig && generate_animation) {
//				mDeepMotionApiClient.generate_animation_async(std::move(original_model_stream), "DummyModel", "fbx", animation_prompt, [this, container, callback, generate_rig, generate_animation](std::stringstream animated_model_stream, const std::string& error_message) {
//					if (error_message.empty()) {
//						std::cout << "Mesh animation successful" << std::endl;
//						
//						callback(std::move(animated_model_stream), "");
//						
//					} else {
//						callback(std::move(container->stream), "");
//					}
//				});
//			} else {
//				callback(std::move(model_stream), "");
//			}
			
			callback(std::move(container->stream), "");
			
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
