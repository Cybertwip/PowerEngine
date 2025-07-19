#pragma once

#include <string>
#include <string_view>

class ModelMetadataComponent {
public:
    ModelMetadataComponent(const std::string& modelPath) 
	: mModelPath(modelPath) {
    }
	
	std::string_view model_path() const {
		return mModelPath;
	}

private:
	std::string mModelPath;
};
