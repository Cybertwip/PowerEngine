// Include your internal structures
#include "filesystem/MeshActorImporter.hpp" // Adjust the path as needed
#include "graphics/shading/MeshData.hpp"
#include "animation/Skeleton.hpp"
#include "animation/Animation.hpp"

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <cassert>

namespace sfbx {

class MeshActorExporter {
public:
	MeshActorExporter();
	~MeshActorExporter();
	
	bool exportActor(const MeshActorImporter::CompressedMeshActor& actor, const std::string& exportPath);
	
private:
	// FBX Document
	sfbx::Document mDocument;
	
	// Internal Data
	std::vector<std::unique_ptr<MeshData>> mMeshes;
	std::unique_ptr<Skeleton> mSkeleton;
	// Animations are skipped as per user request
	
	// Helper Functions
	void createMaterials(const std::vector<std::shared_ptr<SerializableMaterialProperties>>& materials, std::map<int, std::shared_ptr<sfbx::Material>>& materialMap);
	void createMesh(MeshData& meshData, const std::map<int, std::shared_ptr<sfbx::Material>>& materialMap, std::shared_ptr<sfbx::Mesh> parentModel);
	
	// Deserialization Functions (to be implemented)
	bool deserializeMeshes(const CompressedSerialization::Deserializer& deserializer);
	bool deserializeSkeleton(const CompressedSerialization::Deserializer& deserializer);
};

} // namespace sfbx

