#pragma once

#include "filesystem/CompressedSerialization.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

class MeshActorImporter {
public:
	struct CompressedMeshActor {
		struct CompressedAsset {
			std::unique_ptr<	CompressedSerialization::Serializer> mSerializer;
			
			void persist() {
				
				// Ensure destination path exists
				std::filesystem::path destPath(mPrecomputedPath);
				if (!std::filesystem::exists(mPrecomputedPath)) {
					std::filesystem::create_directories(mPrecomputedPath);
				}

				mSerializer->save_to_file(mPrecomputedPath);
			}
			
			std::string mPrecomputedPath;
		};

		CompressedAsset mMesh;
		std::optional<std::vector<CompressedAsset>> mAnimations;
		
		void persist(bool persistAnimations) {
			mMesh.persist();
			
			if (persistAnimations) {
				if (mAnimations.has_value()) {
					for (auto& animation : *mAnimations) {
						animation.persist();
					}
				}
			}
		}
	};
	
	MeshActorImporter();
	
	std::unique_ptr<CompressedMeshActor> process(const std::string& path, const std::string& destination);
};
