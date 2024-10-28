#include "DeepMotionApiClient.hpp"

#include "filesystem/CompressedSerialization.hpp"

#include <cctype>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <future>
#include <thread>
#include <functional>

// Include other necessary headers
#include <httplib.h>
#include <json/json.h>

// Namespace for helper functions and constants
namespace DeepMotionUtils {
// Base64 encoding table
const std::string base64_chars =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";

/**
 * @brief Helper function to check if a character is Base64.
 *
 * @param c The character to check.
 * @return true If the character is a valid Base64 character.
 * @return false Otherwise.
 */
inline bool is_base64(uint8_t c) {
	return (std::isalnum(c) || (c == '+') || (c == '/'));
}
} // namespace DeepMotionUtils

DeepMotionApiClient::DeepMotionApiClient()
: client_(nullptr), authenticated_(false) {}

DeepMotionApiClient::~DeepMotionApiClient() {
	// Destructor can be used to clean up resources if needed
}

// ---------------------
// Private Helper Methods
// ---------------------

std::string DeepMotionApiClient::base64_encode(const std::string& input) const {
	std::string ret;
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];
	
	size_t in_len = input.size();
	size_t pos = 0;
	
	while (in_len--) {
		char_array_3[i++] = input[pos++];
		if (i == 3) {
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) +
			((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) +
			((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;
			
			for (i = 0; i < 4; ++i)
				ret += DeepMotionUtils::base64_chars[char_array_4[i]];
			i = 0;
		}
	}
	
	if (i) {
		for (j = i; j < 3; j++)
			char_array_3[j] = '\0';
		
		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) +
		((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) +
		((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;
		
		for (j = 0; j < i + 1; j++)
			ret += DeepMotionUtils::base64_chars[char_array_4[j]];
		
		while ((i++ < 3))
			ret += '=';
	}
	
	return ret;
}

bool DeepMotionApiClient::read_file(const std::string& file_path, std::vector<char>& data) const {
	std::ifstream file(file_path, std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		std::cerr << "Failed to open file for reading: " << file_path << std::endl;
		return false;
	}
	
	std::streamsize size = file.tellg();
	if (size < 0) {
		std::cerr << "Failed to get file size: " << file_path << std::endl;
		file.close();
		return false;
	}
	
	file.seekg(0, std::ios::beg);
	data.resize(static_cast<size_t>(size));
	if (!file.read(data.data(), size)) {
		std::cerr << "Failed to read file data: " << file_path << std::endl;
		file.close();
		return false;
	}
	
	file.close();
	return true;
}

std::string DeepMotionApiClient::to_uppercase(const std::string& input) const {
	std::string result = input;
	std::transform(result.begin(), result.end(), result.begin(),
				   [](unsigned char c) { return std::toupper(c); });
	return result;
}

std::string DeepMotionApiClient::store_model_internal(const std::string& model_url, const std::string& model_name) {
	// Construct JSON payload
	Json::Value post_json_data;
	post_json_data["modelUrl"] = model_url;
	post_json_data["modelName"] = model_name;
	
	Json::StreamWriterBuilder writer;
	std::string json_payload = Json::writeString(writer, post_json_data);
	
	std::string path = "/character/v1/storeModel";
	std::string content_type = "application/json";
	
	auto res = client_->Post(path.c_str(), json_payload, content_type.c_str());
	
	if (res && res->status == 200) {
		// Parse JSON response to extract modelId
		Json::CharReaderBuilder reader_builder;
		std::unique_ptr<Json::CharReader> reader(reader_builder.newCharReader());
		Json::Value json_data;
		std::string errors;
		
		bool parsing_successful = reader->parse(res->body.c_str(),
												res->body.c_str() + res->body.size(),
												&json_data,
												&errors);
		if (parsing_successful && json_data.isMember("modelId")) {
			return json_data["modelId"].asString();
		} else {
			std::cerr << "Failed to parse JSON response or 'modelId' not found." << std::endl;
			return "";
		}
	} else {
		std::cerr << "Failed to store model. HTTP Status: " << (res ? std::to_string(res->status) : "No Response") << std::endl;
	}
	
	return "";
}

// ---------------------
// Public Methods
// ---------------------

bool DeepMotionApiClient::authenticate(const std::string& api_base_url, int api_base_port,
									   const std::string& client_id, const std::string& client_secret) {
	std::lock_guard<std::mutex> lock(client_mutex_);
	
	// Encode client_id and client_secret in Base64
	std::string credentials = client_id + ":" + client_secret;
	std::string encoded_credentials = base64_encode(credentials);
	
	// Initialize or update the HTTP client with the new API base URL
	client_ = std::make_unique<httplib::SSLClient>(api_base_url.c_str(), api_base_port);
	if (!client_) {
		std::cerr << "Failed to create HTTP client." << std::endl;
		return false;
	}
	
	// Set Authorization header
	httplib::Headers headers = {
		{ "Authorization", "Basic " + encoded_credentials }
	};
	client_->set_default_headers(headers);
	
	// Perform authentication request
	auto res = client_->Get("/account/v1/auth"); // Replace with your auth endpoint
	
	if (res && res->status == 200) {
		auto it = res->headers.find("Set-Cookie");
		if (it != res->headers.end()) {
			session_cookie_ = it->second;
			
			// Extract the 'dmsess' cookie value
			std::size_t start_pos = session_cookie_.find("dmsess=");
			if (start_pos != std::string::npos) {
				std::size_t end_pos = session_cookie_.find(";", start_pos);
				if (end_pos != std::string::npos) {
					session_cookie_ = session_cookie_.substr(start_pos, end_pos - start_pos);
				} else {
					session_cookie_ = session_cookie_.substr(start_pos);
				}
			}
			
			// Set cookie header for future requests if needed
			httplib::Headers cookie_headers = {
				{ "cookie", session_cookie_ }
			};
			client_->set_default_headers(cookie_headers);
			
			authenticated_ = true;
			return true;
		}
	}
	
	authenticated_ = false;
	return false;
}

std::string DeepMotionApiClient::get_session_cookie() const {
	std::lock_guard<std::mutex> lock(client_mutex_);
	return session_cookie_;
}

bool DeepMotionApiClient::is_authenticated() const {
	std::lock_guard<std::mutex> lock(client_mutex_);
	return authenticated_;
}

std::string DeepMotionApiClient::process_text_to_motion(const std::string& prompt, const std::string& model_id) {
	std::lock_guard<std::mutex> lock(client_mutex_);
	
	if (!authenticated_) {
		std::cerr << "Client not authenticated." << std::endl;
		return "";
	}
	
	Json::Value post_json_data;
	
	auto params = std::vector<std::string>{
		"prompt=" + prompt,
		"model=" + model_id
	};
	
	for (const auto& param : params) {
		post_json_data["params"].append(param);
	}
	
	Json::StreamWriterBuilder writer;
	std::string json_payload = Json::writeString(writer, post_json_data);
	
	std::string path = "/job/v1/process/text2motion";
	std::string content_type = "application/json";
	
	auto res = client_->Post(path.c_str(), json_payload, content_type.c_str());
	
	if (res && res->status == 200) {
		// Parse JSON response to extract request ID
		Json::CharReaderBuilder reader_builder;
		std::unique_ptr<Json::CharReader> reader(reader_builder.newCharReader());
		Json::Value json_data;
		std::string errors;
		
		bool parsing_successful = reader->parse(res->body.c_str(),
												res->body.c_str() + res->body.size(),
												&json_data,
												&errors);
		if (parsing_successful && json_data.isMember("rid")) {
			return json_data["rid"].asString();
		} else {
			std::cerr << "Failed to parse JSON response or 'rid' not found." << std::endl;
		}
	} else {
		std::cerr << "Failed to process text to motion. HTTP Status: " << (res ? std::to_string(res->status) : "No Response") << std::endl;
	}
	
	return "";
}

Json::Value DeepMotionApiClient::check_job_status(const std::string& request_id) {
	std::lock_guard<std::mutex> lock(client_mutex_);
	
	Json::Value empty_response;
	
	if (!authenticated_) {
		std::cerr << "Client not authenticated." << std::endl;
		return empty_response;
	}
	
	std::string path = "/job/v1/status/" + request_id;
	
	auto res = client_->Get(path.c_str());
	
	if (res && res->status == 200) {
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
			std::cerr << "Failed to parse JSON response for job status." << std::endl;
		}
	} else {
		std::cerr << "Failed to check job status. HTTP Status: " << (res ? std::to_string(res->status) : "No Response") << std::endl;
	}
	
	return empty_response;
}

Json::Value DeepMotionApiClient::download_job_results(const std::string& request_id) {
	std::lock_guard<std::mutex> lock(client_mutex_);
	
	Json::Value empty_response;
	
	if (!authenticated_) {
		std::cerr << "Client not authenticated." << std::endl;
		return empty_response;
	}
	
	std::string path = "/job/v1/download/" + request_id;
	
	auto res = client_->Get(path.c_str());
	
	if (res && res->status == 200) {
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
			std::cerr << "Failed to parse JSON response for job download." << std::endl;
		}
	} else {
		std::cerr << "Failed to download job results. HTTP Status: " << (res ? std::to_string(res->status) : "No Response") << std::endl;
	}
	
	return empty_response;
}

std::string DeepMotionApiClient::get_model_upload_url(bool resumable, const std::string& model_ext) {
	std::lock_guard<std::mutex> lock(client_mutex_);
	
	if (!authenticated_) {
		std::cerr << "Client not authenticated." << std::endl;
		return "";
	}
	
	std::string path = "/character/v1/getModelUploadUrl?resumable=" + std::string(resumable ? "1" : "0") + "&modelExt=" + model_ext;
	
	auto res = client_->Get(path.c_str());
	
	if (res && res->status == 200) {
		// Parse JSON response to extract modelUrl
		Json::CharReaderBuilder reader_builder;
		std::unique_ptr<Json::CharReader> reader(reader_builder.newCharReader());
		Json::Value json_data;
		std::string errors;
		
		bool parsing_successful = reader->parse(res->body.c_str(),
												res->body.c_str() + res->body.size(),
												&json_data,
												&errors);
		if (parsing_successful && json_data.isMember("modelUrl")) {
			return json_data["modelUrl"].asString();
		} else {
			std::cerr << "Failed to parse JSON response or 'modelUrl' not found." << std::endl;
		}
	} else {
		std::cerr << "Failed to get model upload URL. HTTP Status: " << (res ? std::to_string(res->status) : "No Response") << std::endl;
	}
	
	return "";
}

std::string DeepMotionApiClient::store_model(const std::string& model_url, const std::string& model_name) {
	std::lock_guard<std::mutex> lock(client_mutex_);
	
	if (!authenticated_) {
		std::cerr << "Client not authenticated." << std::endl;
		return "";
	}
	
	return store_model_internal(model_url, model_name);
}

Json::Value DeepMotionApiClient::list_models() {
	std::lock_guard<std::mutex> lock(client_mutex_);
	
	Json::Value empty_response;
	
	if (!authenticated_) {
		std::cerr << "Client not authenticated." << std::endl;
		return empty_response;
	}
	
	std::string path = "/character/v1/listModels";
	
	auto res = client_->Get(path.c_str());
	
	if (res && res->status == 200) {
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
			std::cerr << "Failed to parse JSON response for list models." << std::endl;
		}
	} else {
		std::cerr << "Failed to list models. HTTP Status: " << (res ? std::to_string(res->status) : "No Response") << std::endl;
	}
	
	return empty_response;
}

// ---------------------
// Asynchronous Methods Implementation
// ---------------------

void DeepMotionApiClient::authenticate_async(const std::string& api_base_url, int api_base_port,
											 const std::string& client_id, const std::string& client_secret,
											 AuthCallback callback) {
	// Launch asynchronous task
	std::thread([this, api_base_url, api_base_port, client_id, client_secret, callback]() {
		bool success = authenticate(api_base_url, api_base_port, client_id, client_secret);
		if (callback) {
			if (success) {
				callback(true, "");
			} else {
				callback(false, "Authentication failed.");
			}
		}
	}).detach();
}

void DeepMotionApiClient::process_text_to_motion_async(const std::string& prompt, const std::string& model_id,
													   ProcessMotionCallback callback) {
	// Launch asynchronous task
	std::thread([this, prompt, model_id, callback]() {
		std::string request_id = process_text_to_motion(prompt, model_id);
		if (callback) {
			if (!request_id.empty()) {
				callback(request_id, "");
			} else {
				callback("", "Failed to process text to motion.");
			}
		}
	}).detach();
}

std::string DeepMotionApiClient::upload_model(std::stringstream& model_stream, const std::string& model_name, const std::string& model_ext) {
	std::unique_lock<std::mutex> lock(client_mutex_);
	
	if (!authenticated_) {
		std::cerr << "Client not authenticated. Please authenticate before uploading models." << std::endl;
		return "";
	}
	
	// Validate model data
	if (model_stream.str().empty()) {
		std::cerr << "Model data is empty." << std::endl;
		return "";
	}
	
	// Validate model extension
	if (model_ext.empty()) {
		std::cerr << "Model extension is empty." << std::endl;
		return "";
	}
	
	lock.unlock();
	// Step 1: Get the model upload URL
	std::string upload_url = get_model_upload_url(false, model_ext);
	
	lock.lock();
	
	if (upload_url.empty()) {
		std::cerr << "Failed to obtain model upload URL." << std::endl;
		return "";
	}
	
	// Step 2: Parse the upload URL to get host and path
	std::string protocol, host, upload_path;
	std::size_t protocol_pos = upload_url.find("://");
	if (protocol_pos != std::string::npos) {
		protocol = upload_url.substr(0, protocol_pos);
		protocol_pos += 3; // Move past "://"
	} else {
		std::cerr << "Invalid upload URL format: " << upload_url << std::endl;
		return "";
	}
	
	std::size_t host_pos = upload_url.find("/", protocol_pos);
	if (host_pos != std::string::npos) {
		host = upload_url.substr(protocol_pos, host_pos - protocol_pos);
		upload_path = upload_url.substr(host_pos);
	} else {
		host = upload_url.substr(protocol_pos);
		upload_path = "/";
	}
	
	// Step 3: Upload the model via PUT request
	// Determine the port based on the protocol
	int port = 443; // Default HTTPS port
	if (protocol == "http") {
		port = 80;
	}
	
	httplib::SSLClient upload_client(host.c_str(), port);
	upload_client.set_compress(false);
	
	std::string content_type = "application/octet-stream";
	
	std::string model_data_str = model_stream.str();
	auto res = upload_client.Put(upload_path.c_str(),
								 model_data_str.c_str(),
								 model_data_str.size(),
								 content_type.c_str());
	
	if (!res) {
		std::cerr << "Failed to upload model. Network error." << std::endl;
		return "";
	}
	
	if (res->status != 200 && res->status != 201) {
		std::cerr << "Failed to upload model. HTTP Status: " << res->status << std::endl;
		return "";
	}
	
	lock.unlock();
	// Step 4: Store the model using the API
	std::string modelId = store_model_internal(upload_url, model_name);
	lock.lock();
	
	if (modelId.empty()) {
		std::cerr << "Failed to store the uploaded model." << std::endl;
		return "";
	}
	
	std::cout << "Model uploaded and stored successfully: " << model_name << std::endl;
	return modelId;
}

void DeepMotionApiClient::upload_model_async(std::stringstream model_stream, const std::string& model_name,
											 const std::string& model_ext, UploadModelCallback callback) {
	// Launch asynchronous task
	std::thread([this, stream = std::move(model_stream), model_name, model_ext, callback]() mutable {
		std::string model_id = upload_model(stream, model_name, model_ext);
		if (callback) {
			if (!model_id.empty()) {
				callback(model_id, "");
			} else {
				callback("", "Failed to upload model.");
			}
		}
	}).detach();
}

void DeepMotionApiClient::check_job_status_async(const std::string& request_id, StatusCallback callback) {
	// Launch asynchronous task
	std::thread([this, request_id, callback]() {
		Json::Value status = check_job_status(request_id);
		if (callback) {
			if (!status.empty()) {
				callback(status, "");
			} else {
				callback(status, "Failed to check job status.");
			}
		}
	}).detach();
}

void DeepMotionApiClient::download_job_results_async(const std::string& request_id, DownloadCallback callback) {
	// Launch asynchronous task
	std::thread([this, request_id, callback]() {
		Json::Value results = download_job_results(request_id);
		if (callback) {
			if (!results.empty()) {
				callback(results, "");
			} else {
				callback(results, "Failed to download job results.");
			}
		}
	}).detach();
}

void DeepMotionApiClient::list_models_async(ModelsListCallback callback) {
	// Launch asynchronous task
	std::thread([this, callback]() {
		Json::Value models = list_models();
		if (callback) {
			if (!models.empty()) {
				callback(models, "");
			} else {
				callback(models, "Failed to list models.");
			}
		}
	}).detach();
}

void DeepMotionApiClient::animate_model_async(const std::string& prompt, const std::string& model_id,
											  AnimateModelCallback callback) {
	// Launch asynchronous task
	std::thread([this, prompt, model_id, callback]() {
		// Step 1: Process text to motion
		process_text_to_motion_async(prompt, model_id, [this, callback](const std::string& request_id, const std::string& error) {
			if (!error.empty()) {
				std::cerr << "Failed to process text to motion: " << error << std::endl;
				callback(Json::Value(), "Failed to process text to motion.");
				return;
			}
			
			std::cout << "Text to motion request submitted. Request ID: " << request_id << std::endl;
			
			// Step 2: Start polling job status
			std::thread([this, request_id, callback]() {
				poll_status_animate_model(request_id, callback);
			}).detach();
		});
	}).detach();
}

void DeepMotionApiClient::poll_status_animate_model(const std::string& request_id, AnimateModelCallback callback) {
	check_job_status_async(request_id, [this, request_id, callback](const Json::Value& status, const std::string& error) {
		if (!error.empty()) {
			std::cerr << "Failed to check job status: " << error << std::endl;
			// Retry after delay
			std::this_thread::sleep_for(std::chrono::seconds(3));
			nanogui::async([this, request_id, callback]() {
				poll_status_animate_model(request_id, callback);
			});
			return;
		}
		
		if (status.empty()) {
			// Retry after delay
			std::this_thread::sleep_for(std::chrono::seconds(3));
			nanogui::async([this, request_id, callback]() {
				poll_status_animate_model(request_id, callback);
			});
			return;
		}
		
		std::string job_status_upper;
		std::string job_status = job_status_upper = "WORKING";
		
		if (status["count"].asInt() != 0) {
			auto& firstStatus = *status["status"].begin();
			job_status = firstStatus["status"].asString();
			
			
			// Extract and uppercase the status
			job_status_upper = to_uppercase(job_status);
			
			float total = firstStatus["details"]["total"].asFloat();
			float current = firstStatus["details"]["step"].asFloat();
			
			if (current > total) {
				current = total;
			}
			
			int progress = (current * 100.0f) / total;
			
			std::cout << "Job Status: " << job_status_upper << " (" << progress << "%)" << std::endl;
			
		}

		float total = status["details"]["total"].asFloat();
		float current = status["details"]["step"].asFloat();
		if (current > total) {
			current = total;
		}
		
		int progress = (current * 100.0f) / total;
		
		std::cout << "Job Status: " << job_status_upper << " (" << progress << "%)" << std::endl;
		
		if (job_status_upper == "SUCCESS") {
			// Step 3: Download job results
			download_job_results_async(request_id, [this, callback](const Json::Value& results, const std::string& error) {
				if (!error.empty()) {
					std::cerr << "Failed to download job results: " << error << std::endl;
					callback(Json::Value(), "Failed to download job results.");
					return;
				}
				
				// Assuming 'results' contains the animation data
				callback(results, "");
			});
		} else if (job_status_upper == "FAILURE") {
			std::cerr << "Job failed." << std::endl;
			callback(Json::Value(), "Job failed.");
		} else {
			// Continue polling after delay
			std::this_thread::sleep_for(std::chrono::seconds(3));
			nanogui::async([this, request_id, callback]() {
				poll_status_animate_model(request_id, callback);
			});
		}
	});
}

void DeepMotionApiClient::generate_animation_async(std::stringstream model_stream, const std::string& model_name, const std::string& model_ext,
												   const std::string& prompt, GenerateAnimationCallback callback) {
	// Step 1: Upload the model asynchronously
	upload_model_async(std::move(model_stream), model_name, model_ext,
					   [this, prompt, callback](const std::string& model_id, const std::string& error) {
		if (!error.empty()) {
			std::cerr << "Model upload failed: " << error << std::endl;
			if (callback) {
				callback(std::stringstream(), "Model upload failed: " + error);
			}
			return;
		}
		
		std::cout << "Model uploaded successfully. Model ID: " << model_id << std::endl;
		
		// Step 2: Process text to motion asynchronously
		process_text_to_motion_async(prompt, model_id, [this, callback](const std::string& request_id, const std::string& error) {
			if (!error.empty()) {
				std::cerr << "Text to motion processing failed: " << error << std::endl;
				if (callback) {
					callback(std::stringstream(), "Text to motion processing failed: " + error);
				}
				return;
			}
			
			std::cout << "Text to motion request submitted. Request ID: " << request_id << std::endl;
			
			// Step 3: Start polling job status asynchronously
			std::thread([this, request_id, callback]() {
				poll_status_generate_animation(request_id, callback);
			}).detach();
		});
	});
}

void DeepMotionApiClient::poll_status_generate_animation(const std::string& request_id, GenerateAnimationCallback callback) {
	check_job_status_async(request_id, [this, request_id, callback](const Json::Value& status, const std::string& error) {
		if (!error.empty()) {
			std::cerr << "Failed to check job status: " << error << std::endl;
			// Retry after delay
			std::this_thread::sleep_for(std::chrono::seconds(3));
			nanogui::async([this, request_id, callback]() {
				poll_status_generate_animation(request_id, callback);
			});
			return;
		}
		
		if (status.empty()) {
			// Retry after delay
			std::this_thread::sleep_for(std::chrono::seconds(3));
			nanogui::async([this, request_id, callback]() {
				poll_status_generate_animation(request_id, callback);
			});
			return;
		}
		std::string job_status_upper;
		std::string job_status = job_status_upper = "WORKING";
		
		if (status["count"].asInt() != 0) {
			auto& firstStatus = *status["status"].begin();
			job_status = firstStatus["status"].asString();
			
			
			// Extract and uppercase the status
			job_status_upper = to_uppercase(job_status);

			float total = firstStatus["details"]["total"].asFloat();
			float current = firstStatus["details"]["step"].asFloat();
			
			if (current > total) {
				current = total;
			}
			
			int progress = (current * 100.0f) / total;
			
			std::cout << "Job Status: " << job_status_upper << " (" << progress << "%)" << std::endl;

		}
		
		if (job_status_upper == "SUCCESS") {
			// Step 4: Download job results asynchronously
			download_job_results_async(request_id, [this, callback](const Json::Value& results, const std::string& error) {
				if (!error.empty()) {
					std::cerr << "Failed to download job results: " << error << std::endl;
					if (callback) {
						callback(std::stringstream(), "Failed to download job results.");
					}
					return;
				}
				
				// Assuming 'results' contains the animation data
				if (results.isMember("links")) {
//					std::stringstream fbx_stream;
					
					Json::Value urls = results["links"][0]["urls"]; // variants not implemented yet

					// Iterate over links to find FBX URL
					for (const auto& url : urls) {
						if (url.isMember("files")) {
							for (const auto& file : url["files"]) {
								if (file.isMember("fbx")) {
									std::string download_url = file["fbx"].asString();
									
									std::cout << "Download URL: " << download_url << std::endl;
									
									// Parse the download URL
									std::string protocol, host, path;
									std::size_t protocol_pos = download_url.find("://");
									if (protocol_pos != std::string::npos) {
										protocol = download_url.substr(0, protocol_pos);
										protocol_pos += 3;
									} else {
										std::cerr << "Invalid download URL format: " << download_url << std::endl;
										if (callback) {
											callback(std::stringstream(), "Invalid download URL format.");
										}
										return;
									}
									
									std::size_t host_pos = download_url.find("/", protocol_pos);
									if (host_pos != std::string::npos) {
										host = download_url.substr(protocol_pos, host_pos - protocol_pos);
										path = download_url.substr(host_pos);
									} else {
										host = download_url.substr(protocol_pos);
										path = "/";
									}
									
									// Determine port
									int port = 443; // Default HTTPS
									if (protocol == "http") {
										port = 80;
									}
									
									// Initialize HTTP client for download
									httplib::SSLClient download_client(host.c_str(), port);
									download_client.set_compress(false);
									
									// Perform GET request
									auto res_download = download_client.Get(path.c_str());
									if (res_download && res_download->status == 200) {
										// Assuming the response body contains FBX data
										
										std::vector<unsigned char> zip_data(res_download->body.begin(), res_download->body.end());
										
										// Decompress ZIP data (use your existing decompress_zip_data function)
										std::vector<std::stringstream> animation_files = Zip::decompress(zip_data);

//										fbx_stream = animatio_fles[0];
										
										if (callback) {
											callback(std::move(animation_files[0]), "");
										}
										return;
									} else {
										std::cerr << "Failed to download FBX file. HTTP Status: "
										<< (res_download ? std::to_string(res_download->status) : "No Response") << std::endl;
										if (callback) {
											callback(std::stringstream(), "Failed to download FBX file.");
										}
										return;
									}
								}
							}
						}
					}
					
					// If no FBX file found
					std::cerr << "No FBX file found in job results." << std::endl;
					if (callback) {
						callback(std::stringstream(), "No FBX file found in job results.");
					}
				} else {
					std::cerr << "Invalid job results format." << std::endl;
					if (callback) {
						callback(std::stringstream(), "Invalid job results format.");
					}
				}
			});
		} else if (job_status_upper == "FAILURE") {
			std::cerr << "Job failed." << std::endl;
			if (callback) {
				callback(std::stringstream(), "Job failed.");
			}
		} else {
			// Continue polling after delay
			std::this_thread::sleep_for(std::chrono::seconds(3));
			nanogui::async([this, request_id, callback]() {
				poll_status_generate_animation(request_id, callback);
			});
		}
	});
}
