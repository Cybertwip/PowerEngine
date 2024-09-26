#pragma once

#include <string>

class MeshActorImporter {
public:
	MeshActorImporter();
	
	void process(const std::string& path, const std::string& destination);
};
