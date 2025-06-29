#include "Gr2.hpp"
#include "SkinnedGr2.hpp"
#include "granny.h" // Use the provided granny.h
#include "granny_data_type_definition.h" // Use the provided granny.h
#include <fstream>
#include <iostream>
#include <filesystem>
#include <vector>
#include <map>

// It is assumed you have a "Gr2.hpp" header file. You will need to update
// the declaration of ProcessMesh in it to match the new signature:
//
// From:
// void ProcessMesh(const granny_mesh* mesh, const granny_model* model, MeshData& resultMesh);
//
// To:
// void ProcessMesh(const granny_mesh* mesh, const granny_model* model, MeshData& resultMesh, int materialBaseIndex);


namespace Gr2Util {
// Helper to convert Granny's 4x4 matrix to glm::mat4
inline glm::mat4 GrannyMatrixToGlm(const granny_matrix_4x4& grannyMatrix) {
	glm::mat4 result;
	// Granny matrices are column-major, same as GLM.
	// The original implementation had a bug (transposing the matrix).
	// This is the correct column-major copy.
	memcpy(glm::value_ptr(result), grannyMatrix, sizeof(glm::mat4));
	return result;
}

void CallbackFunc(granny_log_message_type Type,
				  granny_log_message_origin Origin,
				  char const* Message,
				  void* UserData)
{
	char const* TypeString = GrannyGetLogMessageTypeString(Type);
	char const* OriginString = GrannyGetLogMessageOriginString(Origin);
	
	printf("Granny says: %s (%s)\n"": %s\n", TypeString, OriginString, Message);
}

granny_log_callback LogCallback = { CallbackFunc, NULL };

} // namespace Gr2Util

// --- GrannyState RAII Implementation ---
Gr2::GrannyState::GrannyState() {
	// A handler for Granny's log messages can be set here if desired.
	// For example: GrannySetLogCallback(&MyGrannyLogCallback);
	GrannySetLogCallback(&Gr2Util::LogCallback);
	
	// The default allocator is generally fine.
	granny_allocate_callback* alloc;
	granny_deallocate_callback* dealloc;
	GrannyGetAllocator(&alloc, &dealloc);
	GrannySetAllocator(alloc, dealloc);
}

Gr2::GrannyState::~GrannyState() {
	// Cleanup any Granny-specific resources if necessary
}

// --- Gr2 Class Implementation ---
Gr2::Gr2() : mGrannyState(std::make_unique<GrannyState>()) {}
Gr2::~Gr2() = default;

void Gr2::LoadModel(const std::string& path) {
	mPath = path;
	granny_file* file = GrannyReadEntireFile(path.c_str());
	if (!file) {
		std::cerr << "Error: Failed to load GR2 file: " << path << std::endl;
		return;
	}
	ProcessFile(file);
	GrannyFreeFile(file);
}

void Gr2::LoadModel(std::stringstream& data) {
	std::string str = data.str();
	// Granny requires a non-const void* for memory reading, so we cast away constness.
	granny_file* file = GrannyReadEntireFileFromMemory(str.size(), const_cast<void*>(static_cast<const void*>(str.data())));
	if (!file) {
		std::cerr << "Error: Failed to load GR2 from stream." << std::endl;
		return;
	}
	ProcessFile(file);
	GrannyFreeFile(file);
}

void Gr2::ProcessFile(granny_file* file) {
	granny_file_info* info = GrannyGetFileInfo(file);
	if (!info) {
		std::cerr << "Error: Could not get file info." << std::endl;
		return;
	}
	for (int i = 0; i < info->ModelCount; ++i) {
		ProcessModel(info->Models[i]);
	}
}

void Gr2::ProcessModel(const granny_model* model) {
	if (!model) return;
	ProcessMaterials(model);
	int materialBaseIndex = 0;
	for (int i = 0; i < model->MeshBindingCount; ++i) {
		const granny_mesh* mesh = model->MeshBindings[i].Mesh;
		// This now calls the correct version (base or derived) polymorphically
		ProcessMesh(mesh, model, materialBaseIndex);
		if (mesh) {
			materialBaseIndex += mesh->MaterialBindingCount;
		}
	}
	// Handle the case where there is a skeleton but no meshes
	SkinnedGr2* skinnedGr2 = dynamic_cast<SkinnedGr2*>(this);
	if (skinnedGr2 && model->Skeleton && model->MeshBindingCount == 0) {
		skinnedGr2->BuildSkeleton(model->Skeleton);
	}
}
void Gr2::ProcessMesh(const granny_mesh* mesh, const granny_model* model, int materialBaseIndex) {
	if (!mesh || !mesh->PrimaryVertexData) return;
	
	auto& resultMesh = mMeshes.emplace_back(std::make_unique<MeshData>());
	
	auto& vertices = resultMesh->get_vertices();
	auto& indices = resultMesh->get_indices();
	const auto& vertexData = *mesh->PrimaryVertexData;
	
	if (vertexData.VertexCount == 0) {
		return; // Nothing to process
	}
	
	// --- Vertex Layout Patch ---
	int componentCount = 0;
	if (vertexData.VertexType) {
		for (const granny_data_type_definition* def = vertexData.VertexType; def->Type != granny::EndMember; ++def) {
			componentCount++;
		}
	}
	
	if (componentCount == 0) {
		std::cerr << "Error: Source vertex layout has no components." << std::endl;
		return;
	}
	
	std::vector<granny_data_type_definition> patchedSourceLayout;
	patchedSourceLayout.reserve(componentCount + 1);
	
	for (int i = 0; i < componentCount; ++i) {
		granny_data_type_definition newDef = vertexData.VertexType[i];
		newDef.Name = vertexData.VertexComponentNames[i];
		patchedSourceLayout.push_back(newDef);
	}
	
	granny_data_type_definition endMember = {
		GrannyEndMember, // Type
		nullptr,           // Name
		nullptr,           // ReferenceType
		0,                 // ArrayWidth
		{0, 0, 0},         // Extra
		0                  // Ignored__Ignored
	};
	patchedSourceLayout.push_back(endMember);
	
	// Convert the vertex data using the patched layout.
	std::vector<granny_pnt332_vertex> tempVertices(vertexData.VertexCount);
	
	GrannyConvertVertexLayouts(
							   vertexData.VertexCount,
							   patchedSourceLayout.data(),
							   vertexData.Vertices,
							   GrannyPNT332VertexType,
							   tempVertices.data()
							   );
	
	vertices.reserve(vertexData.VertexCount);
	for (const auto& tv : tempVertices) {
		auto vertex = std::make_unique<MeshVertex>();
		vertex->set_position({tv.Position[0], tv.Position[1], tv.Position[2]});
		vertex->set_normal({tv.Normal[0], tv.Normal[1], tv.Normal[2]});
		vertex->set_texture_coords1({tv.UV[0], tv.UV[1]});
		vertex->set_color({1.0f, 1.0f, 1.0f, 1.0f}); // Default color
		vertices.push_back(std::move(vertex));
	}
	
	// Process indices from the primary topology
	const auto& topology = *mesh->PrimaryTopology;
	
	if (topology.IndexCount > 0) {
		indices.resize(topology.IndexCount);
		GrannyConvertIndices(topology.IndexCount, sizeof(granny_int32), topology.Indices, sizeof(uint32_t), indices.data());
	} else if (topology.Index16Count > 0) {
		indices.resize(topology.Index16Count);
		GrannyConvertIndices(topology.Index16Count, sizeof(granny_uint16), topology.Indices16, sizeof(uint32_t), indices.data());
	} else {
		indices.clear(); // No indices
	}
	
	// Process material assignments
	for (int i = 0; i < topology.GroupCount; ++i) {
		const auto& group = topology.Groups[i];
		for (int tri = 0; tri < group.TriCount; ++tri) {
			for (int v = 0; v < 3; ++v) {
				uint32_t vertexIndex = indices[group.TriFirst * 3 + tri * 3 + v];
				if (vertexIndex < vertices.size()) {
					vertices[vertexIndex]->set_material_id(group.MaterialIndex + materialBaseIndex);
				}
			}
		}
	}
}

void Gr2::ProcessMaterials(const granny_model* model) {
	if (!model || model->MeshBindingCount == 0) {
		// This model has no meshes or materials, so we'll assign a default blue material.
		std::vector<std::shared_ptr<SerializableMaterialProperties>> modelMaterials;
		
		auto blueMaterial = std::make_shared<SerializableMaterialProperties>();
		blueMaterial->mIdentifier = std::hash<std::string>{}("DefaultBlueMaterial");
		blueMaterial->mDiffuse = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f); // R, G, B, Alpha
		blueMaterial->mOpacity = 1.0f;
		blueMaterial->mShininess = 32.0f;
		blueMaterial->mHasDiffuseTexture = false; // No texture file, just the solid color.
		
		modelMaterials.push_back(blueMaterial);
		mMaterialProperties.push_back(std::move(modelMaterials));
		return;
	}
	
	std::vector<std::shared_ptr<SerializableMaterialProperties>> serializableMaterials;
	std::map<const granny_material*, uint32_t> uniqueMaterialMap; // To handle shared materials
	
	// Iterate through the meshes of the model
	for (int meshIdx = 0; meshIdx < model->MeshBindingCount; ++meshIdx) {
		const granny_mesh* mesh = model->MeshBindings[meshIdx].Mesh;
		if (!mesh) continue;
		
		// For each mesh, iterate through its material bindings
		for (int i = 0; i < mesh->MaterialBindingCount; ++i) {
			const granny_material* material = mesh->MaterialBindings[i].Material;
			if (!material) continue;
			
			// Only process unique materials to avoid duplicates
			if (uniqueMaterialMap.find(material) != uniqueMaterialMap.end()) {
				serializableMaterials.push_back(serializableMaterials[uniqueMaterialMap[material]]);
				continue;
			}
			
			auto matPtr = std::make_shared<SerializableMaterialProperties>();
			SerializableMaterialProperties& matData = *matPtr;
			
			matData.mIdentifier = std::hash<std::string>{}(material->Name ? material->Name : "");
			matData.mDiffuse = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); // Default
			matData.mOpacity = 1.0f;
			matData.mShininess = 32.0f;
			
			granny_texture* diffuseTexture = GrannyGetMaterialTextureByType(material, GrannyDiffuseColorTexture);
			
			if (diffuseTexture && diffuseTexture->FromFileName) {
				std::filesystem::path gr2FilePath = mPath;
				std::filesystem::path texturePath = gr2FilePath.parent_path() / diffuseTexture->FromFileName;
				
				std::ifstream textureFile(texturePath, std::ios::binary);
				if (textureFile) {
					matData.mTextureDiffuse.assign(std::istreambuf_iterator<char>(textureFile),
												   std::istreambuf_iterator<char>());
					matData.mHasDiffuseTexture = true;
				} else {
					std::cerr << "Warning: Unable to open texture file: " << texturePath << ". ";
					std::cerr << "Falling back to embedded texture data." << std::endl;
					
					if (diffuseTexture->Images[0].MIPLevelCount > 0 && diffuseTexture->Images[0].MIPLevels[0].PixelBytes) {
						granny_texture_mip_level* mip = &diffuseTexture->Images[0].MIPLevels[0];
						const char* pixelData = static_cast<const char*>(mip->PixelBytes);
						matData.mTextureDiffuse.assign(pixelData, pixelData + mip->PixelByteCount);
						matData.mHasDiffuseTexture = true;
					}
				}
			}
			uniqueMaterialMap[material] = serializableMaterials.size();
			serializableMaterials.push_back(matPtr);
		}
	}
	
	mMaterialProperties.push_back(std::move(serializableMaterials));
}

std::vector<std::unique_ptr<MeshData>>& Gr2::GetMeshData() { return mMeshes; }
void Gr2::SetMeshData(std::vector<std::unique_ptr<MeshData>>&& meshData) { mMeshes = std::move(meshData); }
std::vector<std::vector<std::shared_ptr<SerializableMaterialProperties>>>& Gr2::GetMaterialProperties() { return mMaterialProperties; }
bool Gr2::SaveTo(const std::string& filename) const { return false; }
bool Gr2::LoadFrom(const std::string& filename) { return false; }
