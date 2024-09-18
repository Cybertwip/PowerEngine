#include "TranslationGizmo.hpp"
#include "actors/Actor.hpp"
#include "components/TransformComponent.hpp"
#include <cmath>

TranslationGizmo::TranslationGizmo(ShaderManager& shaderManager)
:
mGizmoId(0),
mHoverGizmoId(0),
mScaleFactor(1),
mGizmoShader(std::make_unique<GizmoMesh::GizmoMeshShader>(shaderManager)) {
	mMeshData = create_axis_mesh_data();
	
	// Add plane meshes
	mMeshData.push_back(create_axis_plane_mesh_data(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)));  // Blue plane
	mMeshData.push_back(create_axis_plane_mesh_data(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)));  // Red plane
	mMeshData.push_back(create_axis_plane_mesh_data(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)));  // Green plane
	
	for (auto& data : mMeshData) {
		mTranslationGizmo.push_back(std::make_unique<GizmoMesh>(*data, *mGizmoShader));
	}
	
	// Create rotation matrices for the axes
	nanogui::Matrix4f xRotation = nanogui::Matrix4f::rotate(nanogui::Vector3f(0.0f, 1.0f, 0.0f), glm::radians(0.0f));
	nanogui::Matrix4f yRotation = nanogui::Matrix4f::rotate(nanogui::Vector3f(1.0f, 0.0f, 0.0f), glm::radians(90.0f));
	nanogui::Matrix4f zRotation = nanogui::Matrix4f::rotate(nanogui::Vector3f(0.0f, 0.0f, 1.0f), glm::radians(0.0f));
	nanogui::Matrix4f planeTransform = nanogui::Matrix4f::identity();
	
	mTranslationTransforms.push_back(xRotation);
	mTranslationTransforms.push_back(nanogui::Matrix4f::identity());
	mTranslationTransforms.push_back(zRotation);
	mTranslationTransforms.push_back(planeTransform * nanogui::Matrix4f::translate(nanogui::Vector3f(10.0f, 10.0f, 0.0f))); // Blue plane
	mTranslationTransforms.push_back(planeTransform * nanogui::Matrix4f::translate(nanogui::Vector3f(0.0f, 10.0f, 10.0f)) *
									 nanogui::Matrix4f::rotate(nanogui::Vector3f(0.0f, 1.0f, 0.0f), glm::radians(90.0f)));  // Red plane
	mTranslationTransforms.push_back(planeTransform * nanogui::Matrix4f::translate(nanogui::Vector3f(10.0f, 0.0f, 10.0f)) *
									 nanogui::Matrix4f::rotate(nanogui::Vector3f(1.0f, 0.0f, 0.0f), glm::radians(90.0f)));  // Green plane
	
	
	
	for (int i = 0; i<6; ++i){
		int identifier = static_cast<int>(reinterpret_cast<long long>(&mTranslationTransforms[i]));
		
		mMetadataComponents.push_back({identifier, std::to_string(identifier)});
	}
}

void TranslationGizmo::select(int gizmoId) {
	mGizmoId = gizmoId;
}

void TranslationGizmo::hover(int gizmoId) {
	mHoverGizmoId = gizmoId != 0 ? gizmoId : 0;
}

void TranslationGizmo::transform(std::optional<std::reference_wrapper<Actor>> actor, float px, float py) {
	if (mGizmoId != 0){
		if (mGizmoId == mMetadataComponents[0].identifier()) {
			if (actor.has_value()) {
				auto& transformComponent = actor->get().get_component<TransformComponent>();
				
				auto translation = transformComponent.get_translation();
				
				translation.x += px;

				transformComponent.set_translation(translation);
			}
		} else if (mGizmoId == mMetadataComponents[1].identifier()) {
			if (actor.has_value()) {
				auto& transformComponent = actor->get().get_component<TransformComponent>();
				
				auto translation = transformComponent.get_translation();
				
				translation.y += py;
				
				transformComponent.set_translation(translation);
			}
		} else if (mGizmoId == mMetadataComponents[2].identifier()) {
			if (actor.has_value()) {
				auto& transformComponent = actor->get().get_component<TransformComponent>();
				
				auto translation = transformComponent.get_translation();
				
				translation.z += px;
				
				transformComponent.set_translation(translation);
			}
		}
	}
}

void TranslationGizmo::draw_content( const nanogui::Matrix4f& model, const nanogui::Matrix4f& view, const nanogui::Matrix4f& projection) {
	
}

std::vector<std::unique_ptr<GizmoMesh::MeshData>> TranslationGizmo::create_axis_mesh_data() {
	const int numSegments = 40;
	const float radius = 1;
	const float length = 20;
	const float coneHeight = 5;
	const float coneBaseRadius = 2.5;
	
	std::unique_ptr<GizmoMesh::MeshData> meshDataPtrX = std::make_unique<GizmoMesh::MeshData>();
	std::unique_ptr<GizmoMesh::MeshData> meshDataPtrY = std::make_unique<GizmoMesh::MeshData>();
	std::unique_ptr<GizmoMesh::MeshData> meshDataPtrZ = std::make_unique<GizmoMesh::MeshData>();
	auto& meshDataX = *meshDataPtrX;
	auto& meshDataY = *meshDataPtrY;
	auto& meshDataZ = *meshDataPtrZ;
	
	// Generate cylinder for X axis
	for (int i = 0; i <= numSegments; ++i) {
		float theta = 2.0f * M_PI * float(i) / float(numSegments);
		float y = radius * cosf(theta);
		float z = radius * sinf(theta);
		
		meshDataX.mVertices.push_back(std::make_unique<GizmoMesh::Vertex>(glm::vec3({0.0f, y, z})));
		meshDataX.mVertices.push_back(
									  std::make_unique<GizmoMesh::Vertex>(glm::vec3({length, y, z})));
	}
	
	// Generate cone for X axis arrowhead
	for (int i = 0; i <= numSegments; ++i) {
		float theta = 2.0f * M_PI * float(i) / float(numSegments);
		float y = coneBaseRadius * cosf(theta);
		float z = coneBaseRadius * sinf(theta);
		
		meshDataX.mVertices.push_back(
									  std::make_unique<GizmoMesh::Vertex>(glm::vec3({length, y, z})));
		meshDataX.mVertices.push_back(
									  std::make_unique<GizmoMesh::Vertex>(glm::vec3({length + coneHeight, 0.0f, 0.0f})));
	}
	
	// Generate cylinder for Y axis
	for (int i = 0; i <= numSegments; ++i) {
		float theta = 2.0f * M_PI * float(i) / float(numSegments);
		float x = radius * cosf(theta);
		float z = radius * sinf(theta);
		
		meshDataY.mVertices.push_back(std::make_unique<GizmoMesh::Vertex>(glm::vec3({x, 0.0f, z})));
		meshDataY.mVertices.push_back(
									  std::make_unique<GizmoMesh::Vertex>(glm::vec3({x, length, z})));
	}
	
	// Generate cone for Y axis arrowhead
	for (int i = 0; i <= numSegments; ++i) {
		float theta = 2.0f * M_PI * float(i) / float(numSegments);
		float x = coneBaseRadius * cosf(theta);
		float z = coneBaseRadius * sinf(theta);
		
		meshDataY.mVertices.push_back(
									  std::make_unique<GizmoMesh::Vertex>(glm::vec3({x, length, z})));
		meshDataY.mVertices.push_back(
									  std::make_unique<GizmoMesh::Vertex>(glm::vec3({0.0f, length + coneHeight, 0.0f})));
	}
	
	for (int i = 0; i <= numSegments; ++i) {
		float theta = 2.0f * M_PI * float(i) / float(numSegments);
		float x = radius * cosf(theta);
		float y = radius * sinf(theta);
		
		meshDataZ.mVertices.push_back(std::make_unique<GizmoMesh::Vertex>(glm::vec3({x, y, 0.0f})));
		meshDataZ.mVertices.push_back(
									  std::make_unique<GizmoMesh::Vertex>(glm::vec3({x, y, length})));
	}
	
	for (int i = 0; i <= numSegments; ++i) {
		float theta = 2.0f * M_PI * float(i) / float(numSegments);
		float x = coneBaseRadius * cosf(theta);
		float y = coneBaseRadius * sinf(theta);
		
		meshDataZ.mVertices.push_back(
									  std::make_unique<GizmoMesh::Vertex>(glm::vec3({x, y, length})));
		meshDataZ.mVertices.push_back(
									  std::make_unique<GizmoMesh::Vertex>(glm::vec3({0.0f, 0.0f, length + coneHeight})));
	}
	
	std::vector<std::unique_ptr<GizmoMesh::MeshData>> meshDataVector;
	meshDataVector.push_back(std::move(meshDataPtrX));
	meshDataVector.push_back(std::move(meshDataPtrY));
	meshDataVector.push_back(std::move(meshDataPtrZ));
	
	for (unsigned int k = 0; k < meshDataVector.size(); ++k) {
		for (int i = 0; i <= numSegments; ++i) {
			auto& meshData = meshDataVector[k];
			// Indexing
			int index1 = 2 * i;
			int index2 = 2 * i + 1;
			
			if (i < numSegments) {
				meshData->mIndices.push_back(index1);
				meshData->mIndices.push_back(index1 + 2);
				meshData->mIndices.push_back(index2);
				
				meshData->mIndices.push_back(index2);
				meshData->mIndices.push_back(index1 + 2);
				meshData->mIndices.push_back(index2 + 2);
			}
		}
		
		int baseIndex = 2 * (numSegments + 1);
		for (int i = 0; i <= numSegments; ++i) {
			auto& meshData = meshDataVector[k];
			
			// Indexing
			int index1 = baseIndex + 2 * i;
			int index2 = baseIndex + 2 * i + 1;
			
			if (i < numSegments) {
				meshData->mIndices.push_back(index1);
				meshData->mIndices.push_back(index1 + 2);
				meshData->mIndices.push_back(index2);
			}
		}
	}
	
	meshDataVector[0]->mColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
	meshDataVector[1]->mColor = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
	meshDataVector[2]->mColor = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
	
	return meshDataVector;}

std::unique_ptr<GizmoMesh::MeshData> TranslationGizmo::create_axis_plane_mesh_data(const glm::vec4& color) {
	const float size = 2.5;  // Size of the square
	
	std::unique_ptr<GizmoMesh::MeshData> meshDataPtr = std::make_unique<GizmoMesh::MeshData>();
	auto& meshData = *meshDataPtr;
	
	// Define the vertices of the square
	meshData.mVertices.push_back(
								 std::make_unique<GizmoMesh::Vertex>(glm::vec3({-size / 2, -size / 2, 0.0f})));
	meshData.mVertices.push_back(
								 std::make_unique<GizmoMesh::Vertex>(glm::vec3({size / 2, -size / 2, 0.0f})));
	meshData.mVertices.push_back(
								 std::make_unique<GizmoMesh::Vertex>(glm::vec3({size / 2, size / 2, 0.0f})));
	meshData.mVertices.push_back(
								 std::make_unique<GizmoMesh::Vertex>(glm::vec3({-size / 2, size / 2, 0.0f})));
	
	// Define the indices for the front face (two triangles forming the square)
	meshData.mIndices.push_back(0);
	meshData.mIndices.push_back(1);
	meshData.mIndices.push_back(2);
	meshData.mIndices.push_back(0);
	meshData.mIndices.push_back(2);
	meshData.mIndices.push_back(3);
	
	// Define the indices for the back face (two triangles forming the square)
	meshData.mIndices.push_back(2);
	meshData.mIndices.push_back(1);
	meshData.mIndices.push_back(0);
	meshData.mIndices.push_back(3);
	meshData.mIndices.push_back(2);
	meshData.mIndices.push_back(0);
	
	// Set the color of the square
	meshData.mColor = color;
	
	return meshDataPtr;
}
