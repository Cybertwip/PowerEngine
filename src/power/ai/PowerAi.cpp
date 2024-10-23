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
	// Shared state to track authentication results
	struct AuthState {
		std::mutex mutex;
		int completed = 0;
		bool success = true;
		std::string error_message;
	};
	
	auto state = std::make_shared<AuthState>();
	
	// Lambda to check if all authentications are completed
	auto check_and_callback = [state, callback]() {
		std::lock_guard<std::mutex> lock(state->mutex);
		if (state->completed == 3) { // Total number of services
			callback(state->success, state->error_message);
		}
	};
	
	// Authenticate OpenAI
	mOpenAiApiClient.authenticate_async(openai_api_key, [&](bool success, const std::string& error_message) {
		std::lock_guard<std::mutex> lock(state->mutex);
		if (!success) {
			state->success = false;
			state->error_message += "OpenAI Authentication Failed: " + error_message + "\n";
		}
		state->completed++;
		check_and_callback();
	});
	
	// Authenticate Tripo AI
	mTripoAiApiClient.authenticate_async(tripo_ai_api_key, [&](bool success, const std::string& error_message) {
		std::lock_guard<std::mutex> lock(state->mutex);
		if (!success) {
			state->success = false;
			state->error_message += "Tripo AI Authentication Failed: " + error_message + "\n";
		}
		state->completed++;
		check_and_callback();
	});
	
	// Authenticate DeepMotion
	mDeepMotionApiClient.authenticate_async(deepmotion_api_base_url, deepmotion_api_base_port, deepmotion_client_id, deepmotion_client_secret,
											[&](bool success, const std::string& error_message) {
		std::lock_guard<std::mutex> lock(state->mutex);
		if (!success) {
			state->success = false;
			state->error_message += "DeepMotion Authentication Failed: " + error_message + "\n";
		}
		state->completed++;
		check_and_callback();
	}
											);
}
