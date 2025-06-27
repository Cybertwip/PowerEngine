#pragma once

#include "filesystem/CompressedSerialization.hpp"
#include "graphics/drawing/Mesh.hpp"
#include "graphics/shading/MaterialProperties.hpp"
#include <glm/glm.hpp>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

// Forward declarations for the Granny library types
struct granny_file;
struct granny_file_info;
struct granny_node;
struct granny_mesh;
struct granny_material;
struct granny_model;
struct granny_skeleton;
struct granny_animation;

class Gr2 {
public:
	Gr2();
	virtual ~Gr2();
	
	void LoadModel(const std::string& path);
	void LoadModel(std::stringstream& data);
	
	std::vector<std::unique_ptr<MeshData>>& GetMeshData();
	void SetMeshData(std::vector<std::unique_ptr<MeshData>>&& meshData);
	std::vector<std::vector<std::shared_ptr<SerializableMaterialProperties>>>& GetMaterialProperties();
	
	bool SaveTo(const std::string& filename) const;
	bool LoadFrom(const std::string& filename);
	
protected:
	virtual void ProcessMesh(const granny_mesh* mesh, const granny_model* model, int materialBaseIndex);

	std::string mPath;
	std::vector<std::unique_ptr<MeshData>> mMeshes;
	std::vector<std::vector<std::shared_ptr<SerializableMaterialProperties>>> mMaterialProperties;
	
private:
	void ProcessFile(granny_file* file);
	void ProcessModel(const granny_model* model);
	void ProcessMaterials(const granny_model* model);
	
	struct GrannyState {
		GrannyState();
		~GrannyState();
	};
	std::unique_ptr<GrannyState> mGrannyState;
};
