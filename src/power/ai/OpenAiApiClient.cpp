#include "OpenAiApiClient.hpp"

#include <cctype>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <future>
#include <thread>

namespace DallEUtils {
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

}

OpenAiApiClient::OpenAiApiClient()
: api_key_(""), client_(nullptr) {
	// Default constructor does not initialize the client
}

OpenAiApiClient::~OpenAiApiClient() {
	// Destructor can be used to clean up resources if needed
}

std::string OpenAiApiClient::base64_encode(const std::string& input) const {
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
				ret += DallEUtils::base64_chars[char_array_4[i]];
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
			ret += DallEUtils::base64_chars[char_array_4[j]];
		
		while ((i++ < 3))
			ret += '=';
	}
	
	return ret;
}

bool OpenAiApiClient::save_api_key(const std::string& file_path) const {
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

bool OpenAiApiClient::load_api_key(const std::string& file_path) {
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

void OpenAiApiClient::initialize_client(const std::string& api_key) {
	client_ = std::make_unique<httplib::SSLClient>("api.openai.com", 443);
	client_->set_default_headers({
		{ "Authorization", "Bearer " + api_key },
		{ "Content-Type", "application/json" }
	});
}

bool OpenAiApiClient::authenticate(const std::string& api_key) {
	if (api_key.empty()) {
		std::cerr << "API key is empty. Cannot authenticate." << std::endl;
		return false;
	}
	
	api_key_ = api_key;
	initialize_client(api_key_);
	
	// Test authentication by listing models
	Json::Value models = list_models();
	return !models.empty();
}

void OpenAiApiClient::authenticate_async(const std::string& api_key, AuthCallback callback) {
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

std::string OpenAiApiClient::generate_image(const std::string& prompt) {
	std::lock_guard<std::mutex> lock(client_mutex_);
	
	if (api_key_.empty()) {
		std::cerr << "API key is not set. Please authenticate first." << std::endl;
		return "";
	}
	
	Json::Value post_json_data;
	post_json_data["prompt"] = prompt;
	post_json_data["n"] = 1; // Number of images to generate
	post_json_data["size"] = "1024x1024"; // Image size
	post_json_data["model"] = "dall-e-3";
	
	Json::StreamWriterBuilder writer;
	std::string json_payload = Json::writeString(writer, post_json_data);
	
	auto res = client_->Post("/v1/images/generations", json_payload, "application/json");
	
	if (res) {
		if (res->status == 200) {
			// Parse JSON response to extract image URL
			Json::CharReaderBuilder reader_builder;
			std::unique_ptr<Json::CharReader> reader(reader_builder.newCharReader());
			Json::Value json_data;
			std::string errors;
			
			bool parsing_successful = reader->parse(res->body.c_str(),
													res->body.c_str() + res->body.size(),
													&json_data,
													&errors);
			if (parsing_successful && json_data.isMember("data")) {
				const Json::Value& data = json_data["data"];
				if (data.isArray() && data.size() > 0 && data[0].isMember("url")) {
					return data[0]["url"].asString();
				}
			}
			std::cerr << "Failed to parse JSON response or 'url' not found. Errors: " << errors << std::endl;
		} else if (res->status == 401) {
			std::cerr << "Authentication failed: Invalid API key." << std::endl;
		} else {
			std::cerr << "Failed to generate image. HTTP Status: " << res->status << std::endl;
			std::cerr << "Response Body: " << res->body << std::endl;
		}
	} else {
		std::cerr << "Failed to generate image. No response from server." << std::endl;
	}
	
	return "";
}

void OpenAiApiClient::generate_image_async(const std::string& prompt, GenerateImageCallback callback) {
	// Launch asynchronous task
	std::thread([this, prompt, callback]() {
		std::string image_url = generate_image(prompt);
		if (!image_url.empty()) {
			callback(image_url, "");
		} else {
			callback("", "Failed to generate image.");
		}
	}).detach();
}

Json::Value OpenAiApiClient::list_models() {
	std::lock_guard<std::mutex> lock(client_mutex_);
	
	if (api_key_.empty()) {
		std::cerr << "API key is not set. Please authenticate first." << std::endl;
		return Json::Value();
	}
	
	auto res = client_->Get("/v1/models");
	
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
			std::cerr << "Failed to list models. HTTP Status: " << res->status << std::endl;
			std::cerr << "Response Body: " << res->body << std::endl;
		}
	} else {
		std::cerr << "Failed to list models. No response from server." << std::endl;
	}
	
	return Json::Value(); // Return empty JSON value on failure
}

void OpenAiApiClient::list_models_async(ListModelsCallback callback) {
	// Launch asynchronous task
	std::thread([this, callback]() {
		Json::Value models = list_models();
		if (!models.empty()) {
			callback(models, "");
		} else {
			callback(Json::Value(), "Failed to list models.");
		}
	}).detach();
}

std::string OpenAiApiClient::generate_text(const std::string& prompt, const std::vector<uint8_t>& image_data) {
	std::lock_guard<std::mutex> lock(client_mutex_);
	
	if (api_key_.empty()) {
		std::cerr << "API key is not set. Please authenticate first." << std::endl;
		return "";
	}
	
	// Encode image data to Base64
	std::string image_str(image_data.begin(), image_data.end());
	std::string base64_image = base64_encode(image_str);
	
	// Construct the JSON payload
	Json::Value post_json_data;
	post_json_data["model"] = "gpt-4o-mini";
	
	Json::Value message;
	message["role"] = "user";
	
	Json::Value content;
	
	// Add text content
	Json::Value text_content;
	text_content["type"] = "text";
	text_content["text"] = prompt;
	content.append(text_content);
	
	// Add image URL content
	Json::Value image_content;
	image_content["type"] = "image_url";
	Json::Value image_url_obj;
	image_url_obj["url"] = "data:image/png;base64," + base64_image;
	image_content["image_url"] = image_url_obj;
	content.append(image_content);
	
	message["content"] = content;
	
	Json::Value messages;
	messages.append(message);
	
	post_json_data["messages"] = messages;
	
	Json::StreamWriterBuilder writer;
	std::string json_payload = Json::writeString(writer, post_json_data);
	
	// Send POST request to /v1/chat/completions
	auto res = client_->Post("/v1/chat/completions", json_payload, "application/json");
	
	if (res) {
		if (res->status == 200) {
			// Parse JSON response to extract generated text
			Json::CharReaderBuilder reader_builder;
			std::unique_ptr<Json::CharReader> reader(reader_builder.newCharReader());
			Json::Value json_data;
			std::string errors;
			
			bool parsing_successful = reader->parse(res->body.c_str(),
													res->body.c_str() + res->body.size(),
													&json_data,
													&errors);
			if (parsing_successful && json_data.isMember("choices")) {
				const Json::Value& choices = json_data["choices"];
				if (choices.isArray() && choices.size() > 0 && choices[0].isMember("message") && choices[0]["message"].isMember("content")) {
					return choices[0]["message"]["content"].asString();
				}
			}
			std::cerr << "Failed to parse JSON response or 'content' not found. Errors: " << errors << std::endl;
		} else if (res->status == 401) {
			std::cerr << "Authentication failed: Invalid API key." << std::endl;
		} else {
			std::cerr << "Failed to generate text. HTTP Status: " << res->status << std::endl;
			std::cerr << "Response Body: " << res->body << std::endl;
		}
	} else {
		std::cerr << "Failed to generate text. No response from server." << std::endl;
	}
	
	return "";
}

void OpenAiApiClient::generate_text_async(const std::string& prompt, const std::vector<uint8_t>& image_data, GenerateTextCallback callback) {
	// Launch asynchronous task
	std::thread([this, prompt, image_data, callback]() {
		std::string generated_text = generate_text(prompt, image_data);
		if (!generated_text.empty()) {
			callback(generated_text, "");
		} else {
			callback("", "Failed to generate text.");
		}
	}).detach();
}

std::string OpenAiApiClient::generate_text(const std::string& prompt) {
	std::lock_guard<std::mutex> lock(client_mutex_);
	
	if (api_key_.empty()) {
		std::cerr << "API key is not set. Please authenticate first." << std::endl;
		return "";
	}
	
	// Construct the JSON payload for text-only generation
	Json::Value post_json_data;
	post_json_data["model"] = "gpt-4o-mini";
	
	Json::Value message;
	message["role"] = "user";
	
	Json::Value content;
	
	// Add text content
	Json::Value text_content;
	text_content["type"] = "text";
	text_content["text"] = prompt;
	content.append(text_content);
	
	message["content"] = content;
	
	Json::Value messages;
	messages.append(message);
	
	post_json_data["messages"] = messages;
	
	Json::StreamWriterBuilder writer;
	std::string json_payload = Json::writeString(writer, post_json_data);
	
	// Send POST request to /v1/chat/completions
	auto res = client_->Post("/v1/chat/completions", json_payload, "application/json");
	
	if (res) {
		if (res->status == 200) {
			// Parse JSON response to extract generated text
			Json::CharReaderBuilder reader_builder;
			std::unique_ptr<Json::CharReader> reader(reader_builder.newCharReader());
			Json::Value json_data;
			std::string errors;
			
			bool parsing_successful = reader->parse(res->body.c_str(),
													res->body.c_str() + res->body.size(),
													&json_data,
													&errors);
			if (parsing_successful && json_data.isMember("choices")) {
				const Json::Value& choices = json_data["choices"];
				if (choices.isArray() && choices.size() > 0 && choices[0].isMember("message") && choices[0]["message"].isMember("content")) {
					return choices[0]["message"]["content"].asString();
				}
			}
			std::cerr << "Failed to parse JSON response or 'content' not found. Errors: " << errors << std::endl;
		} else if (res->status == 401) {
			std::cerr << "Authentication failed: Invalid API key." << std::endl;
		} else {
			std::cerr << "Failed to generate text. HTTP Status: " << res->status << std::endl;
			std::cerr << "Response Body: " << res->body << std::endl;
		}
	} else {
		std::cerr << "Failed to generate text. No response from server." << std::endl;
	}
	
	return "";
}

void OpenAiApiClient::generate_text_async(const std::string& prompt, GenerateTextCallback callback) {
	// Launch asynchronous task
	std::thread([this, prompt, callback]() {
		std::string generated_text = generate_text(prompt);
		if (!generated_text.empty()) {
			callback(generated_text, "");
		} else {
			callback("", "Failed to generate text.");
		}
	}).detach();
}