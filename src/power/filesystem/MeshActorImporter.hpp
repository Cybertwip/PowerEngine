#pragma once

#include <string>
#include <vector>

class MeshActorImporter {
public:
	struct CompressedMeshActor {
		std::string mMeshFile;
		std::vector<std::string> mAnimationFiles;
	};
	
	MeshActorImporter();
	
	CompressedMeshActor process(const std::string& path, const std::string& destination);
};
