#pragma once

#include <string>
#include <string_view>

class BlueprintMetadataComponent {
public:
    BlueprintMetadataComponent(const std::string& blueprintPath) 
	: mBlueprintPath(blueprintPath) {
    }
	
	const std::string& blueprint_path() const {
		return mBlueprintPath;
	}

private:
	std::string mBlueprintPath;
};
