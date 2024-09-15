#include "ScaleGizmo.hpp"
#include "actors/Actor.hpp"
#include "components/TransformComponent.hpp"
#include <cmath>

ScaleGizmo::ScaleGizmo(ShaderManager& shaderManager)
:
mGizmoId(0),
mHoverGizmoId(0),
mScaleFactor(1),
mGizmoShader(std::make_unique<GizmoMesh::GizmoMeshShader>(shaderManager)) {
	mMeshData = create_axis_mesh_data();
	
	// Add plane meshes
	mMeshData.push_back(create_axis_plane_mesh_data(glm::vec3(0.0, 0.0, 1.0)));  // Blue plane
	mMeshData.push_back(create_axis_plane_mesh_data(glm::vec3(1.0f, 0.0f, 0.0)));  // Red plane
	mMeshData.push_back(create_axis_plane_mesh_data(glm::vec3(0.0f, 1.0, 0.0)));  // Green plane
	
	for (auto& data : mMeshData) {
		mGizmo.push_back(std::make_unique<GizmoMesh>(*data, *mGizmoShader));
	}
	
	// Create rotation matrices for the axes
	nanogui::Matrix4f xTranslation = nanogui::Matrix4f::translate(nanogui::Vector3f(25.0f, 0.0f, 0.0f));
	nanogui::Matrix4f yTranslation = nanogui::Matrix4f::translate(nanogui::Vector3f(0.0f, 25.0f, 0.0f));
	nanogui::Matrix4f zTranslation = nanogui::Matrix4f::translate(nanogui::Vector3f(0.0f, 0.0f, 25.0f));
	nanogui::Matrix4f planeTransform = nanogui::Matrix4f::identity();
	
	mTransforms.push_back(xTranslation);
	mTransforms.push_back(yTranslation);
	mTransforms.push_back(zTranslation);
	mTransforms.push_back(planeTransform * nanogui::Matrix4f::translate(nanogui::Vector3f(10.0f, 10.0f, 0.0f))); // Blue plane
	mTransforms.push_back(planeTransform * nanogui::Matrix4f::translate(nanogui::Vector3f(0.0f, 10.0f, 10.0f)) *
									 nanogui::Matrix4f::rotate(nanogui::Vector3f(0.0f, 1.0f, 0.0f), glm::radians(90.0f)));  // Red plane
	mTransforms.push_back(planeTransform * nanogui::Matrix4f::translate(nanogui::Vector3f(10.0f, 0.0f, 10.0f)) *
									 nanogui::Matrix4f::rotate(nanogui::Vector3f(1.0f, 0.0f, 0.0f), glm::radians(90.0f)));  // Green plane
	
	
	
	for (int i = 0; i<6; ++i){
		int identifier = static_cast<int>(reinterpret_cast<long long>(&mTransforms[i]));
		
		mMetadataComponents.push_back({identifier, std::to_string(identifier)});
	}
}

void ScaleGizmo::select(int gizmoId) {
	mGizmoId = gizmoId;
}

void ScaleGizmo::hover(int gizmoId) {
	if (gizmoId != 0){
		if (gizmoId == mMetadataComponents[0].identifier()) {
			mHoverGizmoId = gizmoId;
		} else if (gizmoId == mMetadataComponents[1].identifier()) {
			mHoverGizmoId = gizmoId;
		} else if (gizmoId == mMetadataComponents[2].identifier()) {
			mHoverGizmoId = gizmoId;
		}
	} else {
		mHoverGizmoId = gizmoId;
	}
}

void ScaleGizmo::transform(std::optional<std::reference_wrapper<Actor>> actor, float px, float py) {
	if (mGizmoId != 0){
		if (mGizmoId == mMetadataComponents[0].identifier()) {
			if (actor.has_value()) {
				auto& transformComponent = actor->get().get_component<TransformComponent>();
				
				auto scale = transformComponent.get_scale();
				
				scale.x += px;

				transformComponent.set_scale(scale);
			}
		} else if (mGizmoId == mMetadataComponents[1].identifier()) {
			if (actor.has_value()) {
				auto& transformComponent = actor->get().get_component<TransformComponent>();

				auto scale = transformComponent.get_scale();
				
				scale.y += py;
				
				transformComponent.set_scale(scale);
			}
		} else if (mGizmoId == mMetadataComponents[2].identifier()) {
			if (actor.has_value()) {
				auto& transformComponent = actor->get().get_component<TransformComponent>();

				auto scale = transformComponent.get_scale();
				
				scale.z += px;
				
				transformComponent.set_scale(scale);
			}
		}
	}
}

void ScaleGizmo::draw_content(std::optional<std::reference_wrapper<Actor>> actor, const nanogui::Matrix4f& model, const nanogui::Matrix4f& view, const nanogui::Matrix4f& projection) {
	
	if (actor.has_value()) {
		
		auto& transformComponent = actor->get().get_component<TransformComponent>();
		
		auto translation = transformComponent.get_translation();
		auto rotation = transformComponent.get_rotation();
		
		auto viewInverse = view.inverse();
		glm::vec3 cameraPosition(view.m[3][0], view.m[3][1], view.m[3][2]);
		
		// Use GLM for translation matrix
		glm::mat4 actorTranslationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(translation.x, translation.y, -translation.z));

		for (size_t i = 0; i < mGizmo.size(); ++i) {
			// Convert nanogui::Matrix4f to glm::mat4 for consistent operations
			glm::mat4 gizmoModel = TransformComponent::nanogui_to_glm(mTransforms[i]);
			
			float distance = glm::distance(cameraPosition, glm::vec3(translation.x, translation.y, translation.z)); // Now using actor's position for distance
			float visualScaleFactor = mScaleFactor = std::max(0.005f, distance * 0.005f);

			if (mHoverGizmoId != 0){
				if (mHoverGizmoId == mMetadataComponents[i].identifier()) {
					visualScaleFactor *= 1.5f;
				}
			}

			glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(visualScaleFactor, visualScaleFactor, visualScaleFactor));
			
			// Apply transformations in order: rotation, translation, then scale
			gizmoModel = actorTranslationMatrix * gizmoModel * scaleMatrix;
			
			auto gizmoMatrix = TransformComponent::glm_to_nanogui(gizmoModel);
			
			
			mGizmoShader->set_uniform("identifier", mMetadataComponents[i].identifier());
			
			mGizmo[i]->draw_content(gizmoMatrix, view, projection);
		}
	}

}

std::vector<std::unique_ptr<GizmoMesh::MeshData>> ScaleGizmo::create_axis_mesh_data() {
	const float size = 5;  // Size of the cube
	
	std::unique_ptr<GizmoMesh::MeshData> meshDataPtrX = std::make_unique<GizmoMesh::MeshData>();
	std::unique_ptr<GizmoMesh::MeshData> meshDataPtrY = std::make_unique<GizmoMesh::MeshData>();
	std::unique_ptr<GizmoMesh::MeshData> meshDataPtrZ = std::make_unique<GizmoMesh::MeshData>();
	auto& meshDataX = *meshDataPtrX;
	auto& meshDataY = *meshDataPtrY;
	auto& meshDataZ = *meshDataPtrZ;
	
	// Vertices for a cube centered at the origin, size `size`
	std::vector<glm::vec3> cubeVertices = {
		{-size / 2, -size / 2, -size / 2},
		{size / 2, -size / 2, -size / 2},
		{size / 2, size / 2, -size / 2},
		{-size / 2, size / 2, -size / 2},
		{-size / 2, -size / 2, size / 2},
		{size / 2, -size / 2, size / 2},
		{size / 2, size / 2, size / 2},
		{-size / 2, size / 2, size / 2},
	};
	
	// Indices for the cube faces
	std::vector<unsigned int> cubeIndices = {
		0, 1, 2, 2, 3, 0, // front face
		4, 5, 6, 6, 7, 4, // back face
		0, 1, 5, 5, 4, 0, // bottom face
		2, 3, 7, 7, 6, 2, // top face
		0, 3, 7, 7, 4, 0, // left face
		1, 2, 6, 6, 5, 1  // right face
	};
	
	// Assign cube vertices and indices to each axis mesh
	for (const auto& vertex : cubeVertices) {
		meshDataX.mVertices.push_back(std::make_unique<GizmoMesh::Vertex>(vertex));
		meshDataY.mVertices.push_back(std::make_unique<GizmoMesh::Vertex>(vertex));
		meshDataZ.mVertices.push_back(std::make_unique<GizmoMesh::Vertex>(vertex));
	}
	
	meshDataX.mIndices = cubeIndices;
	meshDataY.mIndices = cubeIndices;
	meshDataZ.mIndices = cubeIndices;
	
	// Assign colors
	meshDataX.mColor = glm::vec3(1.0f, 0.0f, 0.0f); // Red for X-axis
	meshDataY.mColor = glm::vec3(0.0f, 1.0f, 0.0f); // Green for Y-axis
	meshDataZ.mColor = glm::vec3(0.0f, 0.0f, 1.0f); // Blue for Z-axis
	
	std::vector<std::unique_ptr<GizmoMesh::MeshData>> meshDataVector;
	meshDataVector.push_back(std::move(meshDataPtrX));
	meshDataVector.push_back(std::move(meshDataPtrY));
	meshDataVector.push_back(std::move(meshDataPtrZ));
	
	return meshDataVector;
}

std::unique_ptr<GizmoMesh::MeshData> ScaleGizmo::create_axis_plane_mesh_data(const glm::vec3& color) {
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
