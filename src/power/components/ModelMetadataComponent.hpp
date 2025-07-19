#pragma once

#include <string>
#include <string_view>

class ModelMetadataComponent {
public:
    ModelMetadataComponent(const std::string& modelPath) 
	: mModelPath(modelPath) {
    }
	
	const std::string& model_path() const {
		return mModelPath;
	}

private:
	std::string mModelPath;
};
