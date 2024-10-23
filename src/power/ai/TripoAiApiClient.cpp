#include "TripoAiApiClient.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <json/json.h>
#include <chrono>
#include <algorithm> // For std::transform

// Helper function to convert a string to uppercase
std::string to_uppercase(const std::string& input) {
	std::string result = input;
	std::transform(result.begin(), result.end(), result.begin(),
				   [](unsigned char c) { return std::toupper(c); });
	return result;
}

TripoAiApiClient::TripoAiApiClient()
: api_key_(""), client_(nullptr) {
	// Default constructor does not initialize the client
}

TripoAiApiClient::~TripoAiApiClient() {
	// Destructor can be used to clean up resources if needed
}

bool TripoAiApiClient::save_api_key(const std::string& file_path) const {
	std::lock_guard<std::mutex> lock(client_mutex_);
	
	if (api_key_.empty()) {
		std::cerr << "API key is empty. Cannot save." << std::endl;
		return false;
	}
	
	std::ofstream ofs(file_path, std::ios::out | std::ios::trunc);
	if (!ofs.is_open()) {
		std::cerr << "Failed to open file for writing: " << file_path << std::endl;
		return false;
	}
	
	ofs << api_key_;
	ofs.close();
	
	std::cout << "API key saved to " << file_path << std::endl;
	return true;
}

bool TripoAiApiClient::load_api_key(const std::string& file_path) {
	std::lock_guard<std::mutex> lock(client_mutex_);
	
	std::ifstream ifs(file_path, std::ios::in);
	if (!ifs.is_open()) {
		std::cerr << "Failed to open file for reading: " << file_path << std::endl;
		return false;
	}
	
	std::stringstream buffer;
	buffer << ifs.rdbuf();
	api_key_ = buffer.str();
	ifs.close();
	
	if (api_key_.empty()) {
		std::cerr << "API key is empty after loading from file." << std::endl;
		return false;
	}
	
	// Initialize the HTTP client with the loaded API key
	initialize_client(api_key_);
	
	std::cout << "API key loaded from " << file_path << std::endl;
	return true;
}

std::string TripoAiApiClient::get_api_key() const {
	std::lock_guard<std::mutex> lock(client_mutex_);
	return api_key_;
}

void TripoAiApiClient::initialize_client(const std::string& api_key_) {
	client_ = std::make_unique<httplib::SSLClient>("api.tripo3d.ai", 443);
	client_->set_default_headers({
		{ "Authorization", "Bearer " + api_key_ },
		{ "Content-Type", "application/json" }
	});
}

bool TripoAiApiClient::authenticate(const std::string& api_key) {
	if (api_key.empty()) {
		std::cerr << "API key is empty. Cannot authenticate." << std::endl;
		return false;
	}
	
	api_key_ = api_key;
	initialize_client(api_key_);
	
	// Test authentication by checking user balance
	Json::Value balance_response;
	balance_response = check_user_balance();
	if (!balance_response.empty()) {
		int code = balance_response.get("code", -1).asInt();
		if (code == 0 && balance_response.isMember("data")) {
			int balance = balance_response["data"].get("balance", 0).asInt();
			int frozen = balance_response["data"].get("frozen", 0).asInt();
			std::cout << "Authentication successful. Balance: " << balance << ", Frozen: " << frozen << std::endl;
			return true;
		}
	}
	
	std::cerr << "Authentication failed." << std::endl;
	return false;
}

void TripoAiApiClient::authenticate_async(const std::string& api_key, AuthCallback callback) {
	// Launch asynchronous task
	std::thread([this, api_key, callback]() {
		bool success = authenticate(api_key);
		if (callback) {
			if (success) {
				callback(true, "");
			} else {
				callback(false, "Authentication failed.");
			}
		}
	}).detach();
}

Json::Value TripoAiApiClient::check_user_balance() {
	Json::Value empty_response;
	
	if (api_key_.empty()) {
		std::cerr << "Client not authenticated." << std::endl;
		return empty_response;
	}
	
	// Endpoint for checking user balance
	std::string endpoint = "/v2/openapi/user/balance";
	
	// Send GET request to the endpoint
	auto res = client_->Get(endpoint.c_str());
	
	if (res) {
		if (res->status == 200) {
			// Parse JSON response
			Json::CharReaderBuilder reader_builder;
			std::unique_ptr<Json::CharReader> reader(reader_builder.newCharReader());
			Json::Value json_data;
			std::string errors;
			
			bool parsing_successful = reader->parse(res->body.c_str(),
													res->body.c_str() + res->body.size(),
													&json_data,
													&errors);
			if (parsing_successful) {
				return json_data;
			} else {
				std::cerr << "Failed to parse JSON response for user balance. Errors: " << errors << std::endl;
			}
		} else if (res->status == 401) {
			std::cerr << "Authentication failed: Invalid API key." << std::endl;
		} else {
			std::cerr << "Failed to check user balance. HTTP Status: " << res->status << std::endl;
			std::cerr << "Response Body: " << res->body << std::endl;
		}
	} else {
		std::cerr << "Failed to check user balance. No response from server." << std::endl;
	}
	
	return empty_response;
}

std::string TripoAiApiClient::generate_model(const std::string& prompt) {
	std::lock_guard<std::mutex> lock(client_mutex_);
	
	if (api_key_.empty()) {
		std::cerr << "API key is not set. Please authenticate first." << std::endl;
		return "";
	}
	
	// Construct the JSON payload
	Json::Value post_json_data;
	post_json_data["model_version"] = "v2.0-20240919";
	post_json_data["type"] = "text_to_model";
	post_json_data["prompt"] = prompt;
	
	Json::StreamWriterBuilder writer;
	std::string json_payload = Json::writeString(writer, post_json_data);
	
	// Send POST request to /v2/openapi/task
	auto res = client_->Post("/v2/openapi/task", json_payload, "application/json");
	
	if (res) {
		if (res->status == 200 || res->status == 201) { // Assuming 201 Created is also possible
			// Parse JSON response to extract task ID
			Json::CharReaderBuilder reader_builder;
			std::unique_ptr<Json::CharReader> reader(reader_builder.newCharReader());
			Json::Value json_data;
			std::string errors;
			
			bool parsing_successful = reader->parse(res->body.c_str(),
													res->body.c_str() + res->body.size(),
													&json_data,
													&errors);
			if (parsing_successful && json_data.isMember("data") && json_data["data"].isMember("task_id")) {
				return json_data["data"]["task_id"].asString();
			}
			std::cerr << "Failed to parse JSON response or 'task_id' not found. Errors: " << errors << std::endl;
		} else if (res->status == 401) {
			std::cerr << "Authentication failed: Invalid API key." << std::endl;
		} else {
			std::cerr << "Failed to generate model. HTTP Status: " << res->status << std::endl;
			std::cerr << "Response Body: " << res->body << std::endl;
		}
	} else {
		std::cerr << "Failed to generate model. No response from server." << std::endl;
	}
	
	return "";
}

void TripoAiApiClient::generate_model_async(const std::string& prompt, GenerateModelCallback callback) {
	// Launch asynchronous task
	std::thread([this, prompt, callback]() {
		std::string task_id = generate_model(prompt);
		if (!task_id.empty()) {
			callback(task_id, "");
		} else {
			callback("", "Failed to generate model.");
		}
	}).detach();
}

std::string TripoAiApiClient::convert_model(const std::string& original_task_id, const std::string& format, bool quad, int face_limit) {
	std::lock_guard<std::mutex> lock(client_mutex_);
	
	if (api_key_.empty()) {
		std::cerr << "API key is not set. Please authenticate first." << std::endl;
		return "";
	}
	
	// Construct the JSON payload
	Json::Value post_json_data;
	post_json_data["type"] = "convert_model";
	post_json_data["format"] = format;
	post_json_data["original_model_task_id"] = original_task_id;
	post_json_data["quad"] = quad;
	post_json_data["face_limit"] = face_limit;
	
	Json::StreamWriterBuilder writer;
	std::string json_payload = Json::writeString(writer, post_json_data);
	
	// Send POST request to /v2/openapi/task
	auto res = client_->Post("/v2/openapi/task", json_payload, "application/json");
	
	if (res) {
		if (res->status == 200 || res->status == 201) { // Assuming 201 Created is also possible
			// Parse JSON response to extract conversion task ID
			Json::CharReaderBuilder reader_builder;
			std::unique_ptr<Json::CharReader> reader(reader_builder.newCharReader());
			Json::Value json_data;
			std::string errors;
			
			bool parsing_successful = reader->parse(res->body.c_str(),
													res->body.c_str() + res->body.size(),
													&json_data,
													&errors);
			if (parsing_successful && json_data.isMember("data") && json_data["data"].isMember("task_id")) {
				return json_data["data"]["task_id"].asString();
			}
			std::cerr << "Failed to parse JSON response or 'task_id' not found. Errors: " << errors << std::endl;
		} else if (res->status == 401) {
			std::cerr << "Authentication failed: Invalid API key." << std::endl;
		} else {
			std::cerr << "Failed to convert model. HTTP Status: " << res->status << std::endl;
			std::cerr << "Response Body: " << res->body << std::endl;
		}
	} else {
		std::cerr << "Failed to convert model. No response from server." << std::endl;
	}
	
	return "";
}

void TripoAiApiClient::convert_model_async(const std::string& original_task_id, const std::string& format, bool quad, int face_limit, ConvertModelCallback callback) {
	// Launch asynchronous task
	std::thread([this, original_task_id, format, quad, face_limit, callback]() {
		std::string convert_task_id = convert_model(original_task_id, format, quad, face_limit);
		if (!convert_task_id.empty()) {
			callback(convert_task_id, "");
		} else {
			callback("", "Failed to convert model.");
		}
	}).detach();
}

void TripoAiApiClient::generate_mesh(const std::string& prompt, const std::string& format, bool quad, int face_limit, ConvertModelCallback callback) {
	// Step 1: Generate the model
	generate_model_async(prompt, [this, format, quad, face_limit, callback](const std::string& model_task_id, const std::string& error) {
		if (!model_task_id.empty()) {
			std::cout << "Model generation task ID: " << model_task_id << std::endl;
			
			// Step 2: Poll the status until completion
			// We'll implement a simple polling mechanism with a fixed interval and maximum retries
			const int polling_interval_seconds = 3;
			const int max_retries = 100; // Adjust as needed
			
			std::thread([this, model_task_id, format, quad, face_limit, callback, polling_interval_seconds, max_retries]() {
				int retries = 0;
				bool completed = false;
				std::string final_status;
				
				while (retries < max_retries && !completed) {
					std::this_thread::sleep_for(std::chrono::seconds(polling_interval_seconds));
					Json::Value status = check_job_status(model_task_id);
					
					if (status.empty()) {
						std::cerr << "Failed to retrieve status for task ID: " << model_task_id << std::endl;
						retries++;
						continue;
					}
					
					std::string task_status = status["status"].asString();
					// Convert status to uppercase to handle case sensitivity
					std::string task_status_upper = to_uppercase(task_status);
					int progress = status.get("progress", 0).asInt();
					
					std::cout << "Model Generation Status: " << task_status_upper << " (" << progress << "%)" << std::endl;
					
					if (task_status_upper == "SUCCESS") {
						completed = true;
						final_status = "SUCCESS";
					} else if (task_status_upper == "FAILURE") {
						completed = true;
						final_status = "FAILURE";
					}
					
					retries++;
				}
				
				if (final_status == "SUCCESS") {
					// Step 3: Convert the model
					convert_model_async(model_task_id, format, quad, face_limit, callback);
				} else if (final_status == "FAILURE") {
					std::cerr << "Model generation task failed for task ID: " << model_task_id << std::endl;
					callback("", "Model generation task failed.");
				} else {
					std::cerr << "Model generation task did not complete within the expected time for task ID: " << model_task_id << std::endl;
					callback("", "Model generation task timed out.");
				}
			}).detach();
		} else {
			std::cerr << "Model generation failed: " << error << std::endl;
			callback("", "Model generation failed.");
		}
	});
}

void TripoAiApiClient::generate_mesh_async(const std::string& prompt, const std::string& format, bool quad, int face_limit, ConvertModelCallback callback) {
	// Launch asynchronous task
	std::thread([this, prompt, format, quad, face_limit, callback]() {
		generate_mesh(prompt, format, quad, face_limit, callback);
	}).detach();
}

Json::Value TripoAiApiClient::check_job_status(const std::string& task_id) {
	std::lock_guard<std::mutex> lock(client_mutex_);
	
	Json::Value empty_response;
	
	if (api_key_.empty()) {
		std::cerr << "Client not authenticated." << std::endl;
		return empty_response;
	}
	
	// Construct the endpoint URL with the task ID
	std::string endpoint = "/v2/openapi/task/" + task_id;
	
	// Send GET request to the endpoint
	auto res = client_->Get(endpoint.c_str());
	
	if (res) {
		if (res->status == 200) {
			// Parse JSON response
			Json::CharReaderBuilder reader_builder;
			std::unique_ptr<Json::CharReader> reader(reader_builder.newCharReader());
			Json::Value json_data;
			std::string errors;
			
			bool parsing_successful = reader->parse(res->body.c_str(),
													res->body.c_str() + res->body.size(),
													&json_data,
													&errors);
			if (parsing_successful && json_data.isMember("data")) {
				return json_data["data"];
			}
			std::cerr << "Failed to parse JSON response or 'data' not found. Errors: " << errors << std::endl;
		} else if (res->status == 401) {
			std::cerr << "Authentication failed: Invalid API key." << std::endl;
		} else {
			std::cerr << "Failed to check job status. HTTP Status: " << res->status << std::endl;
			std::cerr << "Response Body: " << res->body << std::endl;
		}
	} else {
		std::cerr << "Failed to check job status. No response from server." << std::endl;
	}
	
	return empty_response;
}

void TripoAiApiClient::check_job_status_async(const std::string& task_id, StatusCallback callback) {
	// Launch asynchronous task
	std::thread([this, task_id, callback]() {
		Json::Value status = check_job_status(task_id);
		if (!status.empty()) {
			callback(status, "");
		} else {
			callback(status, "Failed to check job status.");
		}
	}).detach();
}
