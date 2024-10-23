#include "PowerAi.hpp"

#include "DeepMotionApiClient.hpp"
#include "OpenAiApiClient.hpp"
#include "TripoAiApiClient.hpp"

#include <iostream>
#include <json/json.h>

PowerAi::PowerAi(OpenAiApiClient& openAiApiClient,
				 TripoAiApiClient& tripoAiApiClient,
				 DeepMotionApiClient& deepMotionApiClient)
: mOpenAiApiClient(openAiApiClient),
mTripoAiApiClient(tripoAiApiClient),
mDeepMotionApiClient(deepMotionApiClient) {
	// Constructor body (if needed)
}

PowerAi::~PowerAi() {
	// Destructor body (if needed)
}

void PowerAi::generate_image_async(const std::string& prompt,
								   std::function<void(const std::string& image_url, const std::string& error_message)> callback) {
	// Call the OpenAiApiClient's asynchronous generate_image method
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

void PowerAi::generate_mesh_async(const std::string& prompt,
								  std::function<void(const std::string& task_id, const std::string& error_message)> callback) {
	// Call the TripoAiApiClient's asynchronous generate_mesh method
	mTripoAiApiClient.generate_mesh_async(prompt, [callback](const std::string& task_id, const std::string& error_message) {
		if (error_message.empty()) {
			std::cout << "Mesh generation task submitted successfully. Task ID: " << task_id << std::endl;
			callback(task_id, "");
		} else {
			std::cerr << "Failed to generate mesh: " << error_message << std::endl;
			callback("", error_message);
		}
	});
}

void PowerAi::generate_3d_animation_async(const std::string& prompt, const std::string& model_id,
										  std::function<void(const Json::Value& animation_data, const std::string& error_message)> callback) {
	// Call the DeepMotionApiClient's asynchronous animate_model method
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
