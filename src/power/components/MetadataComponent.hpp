#pragma once

#include <string>
#include <string_view>

class MetadataComponent {
public:
    MetadataComponent(int identifier, const std::string& name) : mIdentifier(identifier), mName(name) {
    }

	int identifier() const {
		return mIdentifier;
	}

	void name(std::string_view name) {
		mName = name;
	}
	
	std::string_view name() const {
		return mName;
	}

private:
	int mIdentifier;
	std::string mName;
};
