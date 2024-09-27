#pragma once

#include "filesystem/CompressedSerialization.hpp"

#include <string>
#include <vector>

class MeshActorImporter {
public:
	struct CompressedAsset {
		std::unique_ptr<	CompressedSerialization::Serializer> mSerializer;
		
		std::string mPrecomputedPath;
	};
	
	struct CompressedMeshActor {
		CompressedAsset mMesh;
		std::optional<std::vector<CompressedAsset>> mAnimations;
	};
	
	MeshActorImporter();
	
	CompressedMeshActor process(const std::string& path, const std::string& destination);
};
