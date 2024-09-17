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
	auto viewInverse = TransformComponent::nanogui_to_glm(view.inverse());
	glm::vec3 cameraPosition(view.m[3][0], view.m[3][1], view.m[3][2]);
	
	for (size_t i = 0; i < mTranslationGizmo.size(); ++i) {
		// Convert nanogui::Matrix4f to glm::mat4 for consistent operations
		glm::mat4 gizmoModel = TransformComponent::nanogui_to_glm(mTranslationTransforms[i]);
		
		//      float distance = glm::distance(cameraPosition, glm::vec3(translation.x, translation.y, translation.z)); // Now using actor's position for distance
		float visualScaleFactor = mScaleFactor = std::max(0.005f, 0.005f);
		
		if (mHoverGizmoId != 0){
			if (mHoverGizmoId == mMetadataComponents[i].identifier()) {
				visualScaleFactor *= 1.5f;
			}
		}
		
		glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(visualScaleFactor, visualScaleFactor, visualScaleFactor));
		
		//      gizmoModel *= scaleMatrix;
		
		auto gizmoMatrix = TransformComponent::glm_to_nanogui(gizmoModel);
		
		mGizmoShader->set_uniform("identifier", mMetadataComponents[i].identifier());
		
		mTranslationGizmo[i]->draw_content(gizmoMatrix, view, projection);
	}
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
	// Cylinder vertices
	for (int i = 0; i <= numSegments; ++i) {
		float theta = 2.0f * M_PI * float(i) / float(numSegments);
		float y = radius * cosf(theta);
		float z = radius * sinf(theta);
		
		// Vertex at x=0
		meshDataX.mVertices.push_back(std::make_unique<GizmoMesh::Vertex>(glm::vec3({0.0f, y, z})));
		// Vertex at x=length
		meshDataX.mVertices.push_back(std::make_unique<GizmoMesh::Vertex>(glm::vec3({length, y, z})));
	}
	
	// Cone vertices
	int baseIndexCone = meshDataX.mVertices.size();
	for (int i = 0; i <= numSegments; ++i) {
		float theta = 2.0f * M_PI * float(i) / float(numSegments);
		float y = coneBaseRadius * cosf(theta);
		float z = coneBaseRadius * sinf(theta);
		
		// Base circle at x=length
		meshDataX.mVertices.push_back(std::make_unique<GizmoMesh::Vertex>(glm::vec3({length, y, z})));
	}
	
	// Tip vertex
	int tipIndex = meshDataX.mVertices.size();
	meshDataX.mVertices.push_back(std::make_unique<GizmoMesh::Vertex>(glm::vec3({length + coneHeight, 0.0f, 0.0f})));
	
	// Indices for cylinder
	for (int i = 0; i < numSegments; ++i) {
		int idx0 = 2 * i;
		int idx1 = 2 * i + 1;
		int idx2 = 2 * (i + 1);
		int idx3 = 2 * (i + 1) + 1;
		
		// First triangle
		meshDataX.mIndices.push_back(idx0);
		meshDataX.mIndices.push_back(idx2);
		meshDataX.mIndices.push_back(idx1);
		
		// Second triangle
		meshDataX.mIndices.push_back(idx1);
		meshDataX.mIndices.push_back(idx2);
		meshDataX.mIndices.push_back(idx3);
	}
	
	// Indices for cone
	for (int i = 0; i < numSegments; ++i) {
		int idx0 = baseIndexCone + i;
		int idx1 = baseIndexCone + i + 1;
		
		meshDataX.mIndices.push_back(idx0);
		meshDataX.mIndices.push_back(idx1);
		meshDataX.mIndices.push_back(tipIndex);
	}
	
	// Generate cylinder for Y axis
	// Cylinder vertices
	for (int i = 0; i <= numSegments; ++i) {
		float theta = 2.0f * M_PI * float(i) / float(numSegments);
		float x = radius * cosf(theta);
		float z = radius * sinf(theta);
		
		// Vertex at y=0
		meshDataY.mVertices.push_back(std::make_unique<GizmoMesh::Vertex>(glm::vec3({x, 0.0f, z})));
		// Vertex at y=length
		meshDataY.mVertices.push_back(std::make_unique<GizmoMesh::Vertex>(glm::vec3({x, length, z})));
	}
	
	// Cone vertices
	baseIndexCone = meshDataY.mVertices.size();
	for (int i = 0; i <= numSegments; ++i) {
		float theta = 2.0f * M_PI * float(i) / float(numSegments);
		float x = coneBaseRadius * cosf(theta);
		float z = coneBaseRadius * sinf(theta);
		
		// Base circle at y=length
		meshDataY.mVertices.push_back(std::make_unique<GizmoMesh::Vertex>(glm::vec3({x, length, z})));
	}
	
	// Tip vertex
	tipIndex = meshDataY.mVertices.size();
	meshDataY.mVertices.push_back(std::make_unique<GizmoMesh::Vertex>(glm::vec3({0.0f, length + coneHeight, 0.0f})));
	
	// Indices for cylinder
	for (int i = 0; i < numSegments; ++i) {
		int idx0 = 2 * i;
		int idx1 = 2 * i + 1;
		int idx2 = 2 * (i + 1);
		int idx3 = 2 * (i + 1) + 1;
		
		meshDataY.mIndices.push_back(idx0);
		meshDataY.mIndices.push_back(idx2);
		meshDataY.mIndices.push_back(idx1);
		
		meshDataY.mIndices.push_back(idx1);
		meshDataY.mIndices.push_back(idx2);
		meshDataY.mIndices.push_back(idx3);
	}
	
	// Indices for cone
	for (int i = 0; i < numSegments; ++i) {
		int idx0 = baseIndexCone + i;
		int idx1 = baseIndexCone + i + 1;
		
		meshDataY.mIndices.push_back(idx0);
		meshDataY.mIndices.push_back(idx1);
		meshDataY.mIndices.push_back(tipIndex);
	}
	
	// Generate cylinder for Z axis
	// Cylinder vertices
	for (int i = 0; i <= numSegments; ++i) {
		float theta = 2.0f * M_PI * float(i) / float(numSegments);
		float x = radius * cosf(theta);
		float y = radius * sinf(theta);
		
		// Vertex at z=0
		meshDataZ.mVertices.push_back(std::make_unique<GizmoMesh::Vertex>(glm::vec3({x, y, 0.0f})));
		// Vertex at z=length
		meshDataZ.mVertices.push_back(std::make_unique<GizmoMesh::Vertex>(glm::vec3({x, y, length})));
	}
	
	// Cone vertices
	baseIndexCone = meshDataZ.mVertices.size();
	for (int i = 0; i <= numSegments; ++i) {
		float theta = 2.0f * M_PI * float(i) / float(numSegments);
		float x = coneBaseRadius * cosf(theta);
		float y = coneBaseRadius * sinf(theta);
		
		// Base circle at z=length
		meshDataZ.mVertices.push_back(std::make_unique<GizmoMesh::Vertex>(glm::vec3({x, y, length})));
	}
	
	// Tip vertex
	tipIndex = meshDataZ.mVertices.size();
	meshDataZ.mVertices.push_back(std::make_unique<GizmoMesh::Vertex>(glm::vec3({0.0f, 0.0f, length + coneHeight})));
	
	// Indices for cylinder
	for (int i = 0; i < numSegments; ++i) {
		int idx0 = 2 * i;
		int idx1 = 2 * i + 1;
		int idx2 = 2 * (i + 1);
		int idx3 = 2 * (i + 1) + 1;
		
		meshDataZ.mIndices.push_back(idx0);
		meshDataZ.mIndices.push_back(idx2);
		meshDataZ.mIndices.push_back(idx1);
		
		meshDataZ.mIndices.push_back(idx1);
		meshDataZ.mIndices.push_back(idx2);
		meshDataZ.mIndices.push_back(idx3);
	}
	
	// Indices for cone
	for (int i = 0; i < numSegments; ++i) {
		int idx0 = baseIndexCone + i;
		int idx1 = baseIndexCone + i + 1;
		
		meshDataZ.mIndices.push_back(idx0);
		meshDataZ.mIndices.push_back(idx1);
		meshDataZ.mIndices.push_back(tipIndex);
	}
	
	// Set colors
	meshDataX.mColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
	meshDataY.mColor = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
	meshDataZ.mColor = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
	
	std::vector<std::unique_ptr<GizmoMesh::MeshData>> meshDataVector;
	meshDataVector.push_back(std::move(meshDataPtrX));
	meshDataVector.push_back(std::move(meshDataPtrY));
	meshDataVector.push_back(std::move(meshDataPtrZ));
	
	return meshDataVector;
}

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
	
	// Define the indices for the square (two triangles)
	meshData.mIndices.push_back(0);
	meshData.mIndices.push_back(1);
	meshData.mIndices.push_back(2);
	meshData.mIndices.push_back(2);
	meshData.mIndices.push_back(3);
	meshData.mIndices.push_back(0);
	
	// Set the color of the square
	meshData.mColor = color;
	
	return meshDataPtr;
}
