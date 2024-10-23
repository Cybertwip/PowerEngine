#include "TripoAiApiClient.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <json/json.h>
#include <chrono>
#include <algorithm> // For std::transform

// Helper function to convert a string to uppercase
std::string TripoAiApiClient::to_uppercase(const std::string& input) {
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

std::string TripoAiApiClient::animate_rig(const std::string& original_task_id, const std::string& format) {
	std::lock_guard<std::mutex> lock(client_mutex_);
	
	if (api_key_.empty()) {
		std::cerr << "API key is not set. Please authenticate first." << std::endl;
		return "";
	}
	
	// Construct the JSON payload
	Json::Value post_json_data;
	post_json_data["type"] = "animate_rig";
	post_json_data["original_model_task_id"] = original_task_id;
	post_json_data["out_format"] = format;
	
	Json::StreamWriterBuilder writer;
	std::string json_payload = Json::writeString(writer, post_json_data);
	
	// Send POST request to /v2/openapi/task
	auto res = client_->Post("/v2/openapi/task", json_payload, "application/json");
	
	if (res) {
		if (res->status == 200 || res->status == 201) { // Assuming 201 Created is also possible
			// Parse JSON response to extract animate rig task ID
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
			std::cerr << "Failed to parse JSON response or 'task_id' not found for animate_rig. Errors: " << errors << std::endl;
		} else if (res->status == 401) {
			std::cerr << "Authentication failed: Invalid API key." << std::endl;
		} else {
			std::cerr << "Failed to animate rig. HTTP Status: " << res->status << std::endl;
			std::cerr << "Response Body: " << res->body << std::endl;
		}
	} else {
		std::cerr << "Failed to animate rig. No response from server." << std::endl;
	}
	
	return "";
}

void TripoAiApiClient::animate_rig_async(const std::string& original_task_id, const std::string& format, AnimateRigCallback callback) {
	// Launch asynchronous task
	std::thread([this, original_task_id, format, callback]() {
		std::string animate_rig_task_id = animate_rig(original_task_id, format);
		if (!animate_rig_task_id.empty()) {
			callback(animate_rig_task_id, "");
		} else {
			callback("", "Failed to animate rig.");
		}
	}).detach();
}

std::string TripoAiApiClient::animate_retarget(const std::string& original_task_id, const std::string& format, const std::string& animation) {
	std::lock_guard<std::mutex> lock(client_mutex_);
	
	if (api_key_.empty()) {
		std::cerr << "API key is not set. Please authenticate first." << std::endl;
		return "";
	}
	
	// Construct the JSON payload
	Json::Value post_json_data;
	post_json_data["type"] = "animate_retarget";
	post_json_data["original_model_task_id"] = original_task_id;
	post_json_data["out_format"] = format;
	post_json_data["animation"] = animation;
	
	Json::StreamWriterBuilder writer;
	std::string json_payload = Json::writeString(writer, post_json_data);
	
	// Send POST request to /v2/openapi/task
	auto res = client_->Post("/v2/openapi/task", json_payload, "application/json");
	
	if (res) {
		if (res->status == 200 || res->status == 201) { // Assuming 201 Created is also possible
			// Parse JSON response to extract animate retarget task ID
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
			std::cerr << "Failed to parse JSON response or 'task_id' not found for animate_retarget. Errors: " << errors << std::endl;
		} else if (res->status == 401) {
			std::cerr << "Authentication failed: Invalid API key." << std::endl;
		} else {
			std::cerr << "Failed to animate retarget. HTTP Status: " << res->status << std::endl;
			std::cerr << "Response Body: " << res->body << std::endl;
		}
	} else {
		std::cerr << "Failed to animate retarget. No response from server." << std::endl;
	}
	
	return "";
}

void TripoAiApiClient::animate_retarget_async(const std::string& original_task_id, const std::string& format, const std::string& animation, AnimateRetargetCallback callback) {
	// Launch asynchronous task
	std::thread([this, original_task_id, format, animation, callback]() {
		std::string animate_retarget_task_id = animate_retarget(original_task_id, format, animation);
		if (!animate_retarget_task_id.empty()) {
			callback(animate_retarget_task_id, "");
		} else {
			callback("", "Failed to animate retarget.");
		}
	}).detach();
}

void TripoAiApiClient::generate_mesh(const std::string& prompt, const std::string& format, bool quad, int face_limit, bool generate_rig, bool generate_animation, DownloadModelCallback callback) {
	// Step 1: Generate the model
	generate_model_async(prompt, [this, format, quad, face_limit, generate_rig, generate_animation, callback](const std::string& model_task_id, const std::string& error) {
		if (!model_task_id.empty()) {
			std::cout << "Model generation task ID: " << model_task_id << std::endl;
			
			// Step 2: Poll the status until completion
			// Implement a simple polling mechanism with a fixed interval and maximum retries
			const int polling_interval_seconds = 3;
			const int max_retries = 100; // Adjust as needed
			
			std::thread([this, model_task_id, format, quad, face_limit, generate_rig, generate_animation, callback, polling_interval_seconds, max_retries]() {
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
					convert_model_async(model_task_id, format, quad, face_limit, [this, model_task_id, format, quad, face_limit, generate_rig, generate_animation, callback](const std::string& convert_task_id, const std::string& convert_error) {
						if (!convert_task_id.empty()) {
							std::cout << "Model conversion task ID: " << convert_task_id << std::endl;
							
							// Step 4: Poll the conversion status until completion
							const int polling_interval_seconds = 3;
							const int max_retries = 100; // Adjust as needed
							
							std::thread([this, convert_task_id, generate_rig, generate_animation, callback, polling_interval_seconds, max_retries]() {
								int retries = 0;
								bool completed = false;
								std::string final_status;
								Json::Value conversion_response;
								
								while (retries < max_retries && !completed) {
									std::this_thread::sleep_for(std::chrono::seconds(polling_interval_seconds));
									conversion_response = check_job_status(convert_task_id);
									
									if (conversion_response.empty()) {
										std::cerr << "Failed to retrieve status for conversion task ID: " << convert_task_id << std::endl;
										retries++;
										continue;
									}
									
									std::string task_status = conversion_response["status"].asString();
									// Convert status to uppercase to handle case sensitivity
									std::string task_status_upper = to_uppercase(task_status);
									int progress = conversion_response.get("progress", 0).asInt();
									
									std::cout << "Model Conversion Status: " << task_status_upper << " (" << progress << "%)" << std::endl;
									
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
									// Step 5: Optional Rigging
									if (generate_rig) {
										animate_rig_async(conversion_response["task_id"].asString(), conversion_response["out_format"].asString(), [this, convert_task_id, generate_animation, callback](const std::string& animate_rig_task_id, const std::string& rig_error) {
											if (!animate_rig_task_id.empty()) {
												std::cout << "Animate Rig task ID: " << animate_rig_task_id << std::endl;
												
												// Step 6: Poll the animate rig status until completion
												const int polling_interval_seconds = 3;
												const int max_retries = 100; // Adjust as needed
												
												std::thread([this, animate_rig_task_id, generate_animation, callback, polling_interval_seconds, max_retries]() {
													int retries = 0;
													bool completed = false;
													std::string final_status;
													
													while (retries < max_retries && !completed) {
														std::this_thread::sleep_for(std::chrono::seconds(polling_interval_seconds));
														Json::Value status = check_job_status(animate_rig_task_id);
														
														if (status.empty()) {
															std::cerr << "Failed to retrieve status for animate rig task ID: " << animate_rig_task_id << std::endl;
															retries++;
															continue;
														}
														
														std::string task_status = status["status"].asString();
														// Convert status to uppercase to handle case sensitivity
														std::string task_status_upper = to_uppercase(task_status);
														int progress = status.get("progress", 0).asInt();
														
														std::cout << "Animate Rig Status: " << task_status_upper << " (" << progress << "%)" << std::endl;
														
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
														// Step 7: Optional Animation Retargeting
														if (generate_animation) {
															animate_retarget_async(animate_rig_task_id, "FBX", "preset:run", [this, animate_rig_task_id, callback](const std::string& animate_retarget_task_id, const std::string& retarget_error) {
																if (!animate_retarget_task_id.empty()) {
																	std::cout << "Animate Retarget task ID: " << animate_retarget_task_id << std::endl;
																	
																	// Step 8: Poll the animate retarget status until completion
																	const int polling_interval_seconds = 3;
																	const int max_retries = 100; // Adjust as needed
																	
																	std::thread([this, animate_retarget_task_id, callback, polling_interval_seconds, max_retries]() {
																		int retries = 0;
																		bool completed = false;
																		std::string final_status;
																		Json::Value retarget_response;
																		
																		while (retries < max_retries && !completed) {
																			std::this_thread::sleep_for(std::chrono::seconds(polling_interval_seconds));
																			retarget_response = check_job_status(animate_retarget_task_id);
																			
																			if (retarget_response.empty()) {
																				std::cerr << "Failed to retrieve status for animate retarget task ID: " << animate_retarget_task_id << std::endl;
																				retries++;
																				continue;
																			}
																			
																			std::string task_status = retarget_response["status"].asString();
																			// Convert status to uppercase to handle case sensitivity
																			std::string task_status_upper = to_uppercase(task_status);
																			int progress = retarget_response.get("progress", 0).asInt();
																			
																			std::cout << "Animate Retarget Status: " << task_status_upper << " (" << progress << "%)" << std::endl;
																			
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
																			// Step 9: Download the animated retarget model
																			if (retarget_response.isMember("output") && retarget_response["output"].isMember("model")) {
																				std::string model_url = retarget_response["output"]["model"].asString();
																				std::cout << "Animated Retarget Model URL: " << model_url << std::endl;
																				
																				std::stringstream model_stream;
																				if (download_file(model_url, model_stream)) {
																					// Call the callback with the downloaded model
																					callback(std::move(model_stream), "");
																				} else {
																					std::cerr << "Failed to download the animated retarget model from URL: " << model_url << std::endl;
																					callback(std::stringstream(), "Failed to download the animated retarget model.");
																				}
																			} else {
																				std::cerr << "'model' URL not found inside 'output' in animate retarget response." << std::endl;
																				callback(std::stringstream(), "'model' URL not found inside 'output' in animate retarget response.");
																			}
																		} else if (final_status == "FAILURE") {
																			std::cerr << "Animate Retarget task failed for task ID: " << animate_retarget_task_id << std::endl;
																			callback(std::stringstream(), "Animate Retarget task failed.");
																		} else {
																			std::cerr << "Animate Retarget task did not complete within the expected time for task ID: " << animate_retarget_task_id << std::endl;
																			callback(std::stringstream(), "Animate Retarget task timed out.");
																		}
																	}).detach();
																} else {
																	std::cerr << "Animate Retarget failed: " << retarget_error << std::endl;
																	callback(std::stringstream(), "Animate Retarget failed.");
																}
															});
														} else {
															// If no animation, proceed to download the rigged model
															if (conversion_response.isMember("output") && conversion_response["output"].isMember("model")) {
																std::string model_url = conversion_response["output"]["model"].asString();
																std::cout << "Converted Model URL: " << model_url << std::endl;
																
																std::stringstream model_stream;
																if (download_file(model_url, model_stream)) {
																	// Call the callback with the downloaded model
																	callback(std::move(model_stream), "");
																} else {
																	std::cerr << "Failed to download the converted model from URL: " << model_url << std::endl;
																	callback(std::stringstream(), "Failed to download the converted model.");
																}
															} else {
																std::cerr << "'model' URL not found inside 'output' in conversion response." << std::endl;
																callback(std::stringstream(), "'model' URL not found inside 'output' in conversion response.");
															}
														}
													} else if (final_status == "FAILURE") {
														std::cerr << "Animate Rig task failed for task ID: " << animate_rig_task_id << std::endl;
														callback(std::stringstream(), "Animate Rig task failed.");
													} else {
														std::cerr << "Animate Rig task did not complete within the expected time for task ID: " << animate_rig_task_id << std::endl;
														callback(std::stringstream(), "Animate Rig task timed out.");
													}
												}).detach();
											} else {
												std::cerr << "Animate Rig failed: " << rig_error << std::endl;
												callback(std::stringstream(), "Animate Rig failed.");
											}
										});
									} else if (generate_animation) {
										// If rigging is not required but animation is, we need to animate retarget directly from conversion
										animate_retarget_async(convert_task_id, format, "preset:run", [this, convert_task_id, callback](const std::string& animate_retarget_task_id, const std::string& retarget_error) {
											if (!animate_retarget_task_id.empty()) {
												std::cout << "Animate Retarget task ID: " << animate_retarget_task_id << std::endl;
												
												// Poll the animate retarget status until completion
												const int polling_interval_seconds = 3;
												const int max_retries = 100; // Adjust as needed
												
												std::thread([this, animate_retarget_task_id, callback, polling_interval_seconds, max_retries]() {
													int retries = 0;
													bool completed = false;
													std::string final_status;
													Json::Value retarget_response;
													
													while (retries < max_retries && !completed) {
														std::this_thread::sleep_for(std::chrono::seconds(polling_interval_seconds));
														retarget_response = check_job_status(animate_retarget_task_id);
														
														if (retarget_response.empty()) {
															std::cerr << "Failed to retrieve status for animate retarget task ID: " << animate_retarget_task_id << std::endl;
															retries++;
															continue;
														}
														
														std::string task_status = retarget_response["status"].asString();
														// Convert status to uppercase to handle case sensitivity
														std::string task_status_upper = to_uppercase(task_status);
														int progress = retarget_response.get("progress", 0).asInt();
														
														std::cout << "Animate Retarget Status: " << task_status_upper << " (" << progress << "%)" << std::endl;
														
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
														// Step 6: Download the animated retarget model
														if (retarget_response.isMember("output") && retarget_response["output"].isMember("model")) {
															std::string model_url = retarget_response["output"]["model"].asString();
															std::cout << "Animated Retarget Model URL: " << model_url << std::endl;
															
															std::stringstream model_stream;
															if (download_file(model_url, model_stream)) {
																// Call the callback with the downloaded model
																callback(std::move(model_stream), "");
															} else {
																std::cerr << "Failed to download the animated retarget model from URL: " << model_url << std::endl;
																callback(std::stringstream(), "Failed to download the animated retarget model.");
															}
														} else {
															std::cerr << "'model' URL not found inside 'output' in animate retarget response." << std::endl;
															callback(std::stringstream(), "'model' URL not found inside 'output' in animate retarget response.");
														}
													} else if (final_status == "FAILURE") {
														std::cerr << "Animate Retarget task failed for task ID: " << animate_retarget_task_id << std::endl;
														callback(std::stringstream(), "Animate Retarget task failed.");
													} else {
														std::cerr << "Animate Retarget task did not complete within the expected time for task ID: " << animate_retarget_task_id << std::endl;
														callback(std::stringstream(), "Animate Retarget task timed out.");
													}
												}).detach();
											} else {
												std::cerr << "Animate Retarget failed: " << retarget_error << std::endl;
												callback(std::stringstream(), "Animate Retarget failed.");
											}
										});
									} else {
										// If neither rigging nor animation is required, proceed to download the converted model
										if (conversion_response.isMember("output") && conversion_response["output"].isMember("model")) {
											std::string model_url = conversion_response["output"]["model"].asString();
											std::cout << "Converted Model URL: " << model_url << std::endl;
											
											std::stringstream model_stream;
											if (download_file(model_url, model_stream)) {
												// Call the callback with the downloaded model
												callback(std::move(model_stream), "");
											} else {
												std::cerr << "Failed to download the converted model from URL: " << model_url << std::endl;
												callback(std::stringstream(), "Failed to download the converted model.");
											}
										} else {
											std::cerr << "'model' URL not found inside 'output' in conversion response." << std::endl;
											callback(std::stringstream(), "'model' URL not found inside 'output' in conversion response.");
										}
									}
								} else if (final_status == "FAILURE") {
									std::cerr << "Model conversion task failed for task ID: " << convert_task_id << std::endl;
									callback(std::stringstream(), "Model conversion task failed.");
								} else {
									std::cerr << "Model conversion task did not complete within the expected time for task ID: " << convert_task_id << std::endl;
									callback(std::stringstream(), "Model conversion task timed out.");
								}
							}).detach();
						} else {
							std::cerr << "Model conversion failed: " << convert_error << std::endl;
							callback(std::stringstream(), "Model conversion failed.");
						}
					});
				} else if (final_status == "FAILURE") {
					std::cerr << "Model generation task failed for task ID: " << model_task_id << std::endl;
					callback(std::stringstream(), "Model generation task failed.");
				} else {
					std::cerr << "Model generation task did not complete within the expected time for task ID: " << model_task_id << std::endl;
					callback(std::stringstream(), "Model generation task timed out.");
				}
			}).detach();
		} else {
			std::cerr << "Model generation failed: " << error << std::endl;
			callback(std::stringstream(), "Model generation failed.");
		}
	});
}

void TripoAiApiClient::generate_mesh_async(const std::string& prompt, const std::string& format, bool quad, int face_limit, bool generate_rig, bool generate_animation, DownloadModelCallback callback) {
	// Launch asynchronous task
	std::thread([this, prompt, format, quad, face_limit, generate_rig, generate_animation, callback]() {
		generate_mesh(prompt, format, quad, face_limit, generate_rig, generate_animation, callback);
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

bool TripoAiApiClient::download_file(const std::string& url, std::stringstream& data_stream) {
	// Parse the URL to extract host and path
	std::string host, path;
	if (!parse_url(url, host, path)) {
		std::cerr << "Invalid URL: " << url << std::endl;
		return false;
	}
	
	// Determine if the URL uses HTTPS or HTTP
	bool is_https = false;
	if (url.find("https://") == 0) {
		is_https = true;
	} else if (url.find("http://") == 0) {
		std::cerr << "Unsupported URL scheme in URL: " << url << std::endl;
		return false;
	}
	
	// Create an HTTPS client based on the URL scheme
	std::unique_ptr<httplib::SSLClient> http_client =
	std::make_unique<httplib::SSLClient>(host.c_str(), 443);
	
	// Optional: Set timeout or other client settings here
	
	// Send GET request to download the file
	auto res = http_client->Get(path.c_str());
	
	if (res && res->status == 200) {
		data_stream << res->body;
		return true;
	} else {
		std::cerr << "Failed to download file from URL: " << url << std::endl;
		if (res) {
			std::cerr << "HTTP Status: " << res->status << std::endl;
			std::cerr << "Response Body: " << res->body << std::endl;
		} else {
			std::cerr << "No response from server." << std::endl;
		}
		return false;
	}
}

bool TripoAiApiClient::parse_url(const std::string& url, std::string& host, std::string& path) {
	// Simple URL parsing (not handling all edge cases)
	const std::string https_prefix = "https://";
	const std::string http_prefix = "http://";
	size_t pos = 0;
	
	if (url.find(https_prefix) == 0) {
		pos = https_prefix.length();
	} else if (url.find(http_prefix) == 0) {
		pos = http_prefix.length();
	} else {
		return false;
	}
	
	size_t slash_pos = url.find('/', pos);
	if (slash_pos == std::string::npos) {
		host = url.substr(pos);
		path = "/";
	} else {
		host = url.substr(pos, slash_pos - pos);
		path = url.substr(slash_pos);
	}
	
	return true;
}
