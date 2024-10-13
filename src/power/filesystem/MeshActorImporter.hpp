#pragma once

#include "filesystem/CompressedSerialization.hpp"

#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

class MeshActorImporter {
public:
	struct CompressedMeshActor {
		struct CompressedAsset {
			std::unique_ptr<	CompressedSerialization::Serializer> mSerializer;
			
			void persist() {
				
				std::filesystem::path destPath(mPrecomputedPath);
				
				// Get the parent path (directories up to the file, but not including the file itself)
				std::filesystem::path parentPath = destPath.parent_path();
				
				if (!std::filesystem::exists(parentPath)) {
					std::filesystem::create_directories(parentPath);
				}
				
				mSerializer->save_to_file(mPrecomputedPath);
			}
			
			std::string mPrecomputedPath;
		};
		
		CompressedAsset mMesh;
		std::optional<std::vector<CompressedAsset>> mAnimations;
		
		void persist_mesh() {
			mMesh.persist();
		}
		
		void persist_animations() {
			
			if (mAnimations.has_value()) {
				for (auto& animation : *mAnimations) {
					animation.persist();
				}
			}
		}
	};
	
	MeshActorImporter();
	
	std::unique_ptr<CompressedMeshActor> process(const std::string& path, const std::string& destination, bool convertAxis = true);
	
	std::unique_ptr<CompressedMeshActor> process(std::stringstream& data, const std::string& modelName, const std::string& destination, bool convertAxis = true);
};
