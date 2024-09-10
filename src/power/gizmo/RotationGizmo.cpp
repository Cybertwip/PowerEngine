#include "RotationGizmo.hpp"
#include "actors/Actor.hpp"
#include "components/TransformComponent.hpp"

#include <nanogui/opengl.h>

#include <cmath>

// Constructor
RotationGizmo::RotationGizmo(ShaderManager& shaderManager)
: mGizmoId(0),
mHoverGizmoId(0),
mScaleFactor(1.0f),
mGizmoShader(std::make_unique<GizmoMesh::GizmoMeshShader>(shaderManager)) {
	// Create axis rotation meshes
	mMeshData = create_axis_rotation_mesh_data();
	
	// Create gizmo objects for each rotation axis
	for (auto& data : mMeshData) {
		mRotationGizmo.push_back(std::make_unique<GizmoMesh>(*data, *mGizmoShader));
	}
	
	// Initialize rotation transforms
	mRotationTransforms.push_back(nanogui::Matrix4f::rotate(nanogui::Vector3f(1.0f, 0.0f, 0.0f), glm::radians(270.0f)));
	mRotationTransforms.push_back(nanogui::Matrix4f::rotate(nanogui::Vector3f(0.0f, 0.0f, 1.0f), glm::radians(90.0f)));
	mRotationTransforms.push_back(nanogui::Matrix4f::rotate(nanogui::Vector3f(1.0f, 0.0f, 0.0f), glm::radians(0.0f)));
	
	for (int i = 0; i < 3; ++i) {
		int identifier = static_cast<int>(reinterpret_cast<long long>(&mRotationTransforms[i]));
		mMetadataComponents.push_back({identifier, std::to_string(identifier)});
	}
}

void RotationGizmo::select(int gizmoId) {
	mGizmoId = gizmoId;
}

void RotationGizmo::hover(int gizmoId) {
	mHoverGizmoId = gizmoId != 0 ? gizmoId : 0;
}

void RotationGizmo::transform(std::optional<std::reference_wrapper<Actor>> actor, float angle) {
	if (mGizmoId != 0 && actor.has_value()) {
		auto& transformComponent = actor->get().get_component<TransformComponent>();
		
		// Apply rotation based on the selected axis
		if (mGizmoId == mMetadataComponents[0].identifier()) {
			transformComponent.rotate(glm::vec3(0.0f, 0.0f, -1.0f), angle); // Rotate around Z-axis
		} else if (mGizmoId == mMetadataComponents[1].identifier()) {
			transformComponent.rotate(glm::vec3(1.0f, 0.0f, 0.0f), angle); // Rotate around X-axis
		} else if (mGizmoId == mMetadataComponents[2].identifier()) {
			transformComponent.rotate(glm::vec3(0.0f, -1.0f, 0.0f), angle); // Rotate around Y-axis
		}
	}
}

void RotationGizmo::draw_content(std::optional<std::reference_wrapper<Actor>> actor, const nanogui::Matrix4f& model, const nanogui::Matrix4f& view, const nanogui::Matrix4f& projection) {
	
	if (actor.has_value()) {
		
		auto& transformComponent = actor->get().get_component<TransformComponent>();
		
		auto translation = transformComponent.get_translation();
		auto rotation = transformComponent.get_rotation();
		
		glEnable(GL_DEPTH_TEST);
		
		auto viewInverse = view.inverse();
		glm::vec3 cameraPosition(view.m[3][0], view.m[3][1], view.m[3][2]);
		
		// Use GLM for translation matrix
		glm::mat4 actorTranslationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(translation.x, translation.y, -translation.z));
		
		for (size_t i = 0; i < mRotationGizmo.size(); ++i) {
			// Convert nanogui::Matrix4f to glm::mat4 for consistent operations
			glm::mat4 gizmoModel = TransformComponent::nanogui_to_glm(mRotationTransforms[i]);
			
			float distance = glm::distance(cameraPosition, glm::vec3(translation.x, translation.y, translation.z)); // Now using actor's position for distance
			float visualScaleFactor = mScaleFactor = std::max(0.0025f, distance * 0.0025f);
			
			if (mHoverGizmoId != 0){
				if (mHoverGizmoId == mMetadataComponents[i].identifier()) {
					visualScaleFactor *= 1.05f;
				}
			}
			
			glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(visualScaleFactor, visualScaleFactor, visualScaleFactor));
			
			// Apply transformations in order: rotation, translation, then scale
			gizmoModel = actorTranslationMatrix * gizmoModel * scaleMatrix;
			
			auto gizmoMatrix = TransformComponent::glm_to_nanogui(gizmoModel);
			
			
			mGizmoShader->set_uniform("identifier", mMetadataComponents[i].identifier());
			
			mRotationGizmo[i]->draw_content(gizmoMatrix, view, projection);
		}
		
		glDisable(GL_DEPTH_TEST);
	}
	
}

std::vector<std::unique_ptr<GizmoMesh::MeshData>> RotationGizmo::create_axis_rotation_mesh_data() {
	const int numSegments = 90;        // Increased for smoother torus
	const int numRingSegments = 45;    // Increased for smoother inner ring
	const float mainRadius = 4.0f;     // Adjusted to avoid overlap and z-fighting
	
	// Create mesh data for X, Y, and Z tori
	std::unique_ptr<GizmoMesh::MeshData> meshDataPtrX = std::make_unique<GizmoMesh::MeshData>();
	std::unique_ptr<GizmoMesh::MeshData> meshDataPtrY = std::make_unique<GizmoMesh::MeshData>();
	std::unique_ptr<GizmoMesh::MeshData> meshDataPtrZ = std::make_unique<GizmoMesh::MeshData>();
	auto& meshDataX = *meshDataPtrX;
	auto& meshDataY = *meshDataPtrY;
	auto& meshDataZ = *meshDataPtrZ;
	auto createVertices = [&](GizmoMesh::MeshData& meshData, const float ringRadius){
		for (int i = 0; i <= numSegments; ++i) {
			float theta = 2.0f * M_PI * i / numSegments;
			glm::vec3 centerPos = glm::vec3(cos(theta), sin(theta), 0.0f) * mainRadius;
			
			// Only generate for the upper half
			for (int j = 0; j <= numRingSegments / 2; ++j) { // Notice the change here
				float phi = M_PI * j / (numRingSegments / 2); // Adjusted to cover only 0 to π
				glm::vec3 ringPos = centerPos + ringRadius * glm::vec3(cos(phi), 0.0f, sin(phi));
				
				meshData.mVertices.push_back(std::make_unique<GizmoMesh::Vertex>(ringPos));
			}
		}
	};
	
	// Call the function as before
	createVertices(meshDataZ, 38);
	createVertices(meshDataX, 44);
	createVertices(meshDataY, 50);
	
	// Generate indices for the upper half of each torus mesh
	for (auto& meshData : {&meshDataX, &meshDataY, &meshDataZ}) {
		for (int i = 0; i < numSegments; ++i) {
			for (int j = 0; j < numRingSegments / 2; ++j) { // Adjusted loop limit
				int nextI = (i + 1) % numSegments; // For wrapping around the torus
				int index1 = i * (numRingSegments / 2 + 1) + j;
				int index2 = index1 + 1;
				int index3 = nextI * (numRingSegments / 2 + 1) + j;
				int index4 = index3 + 1;
				
				// Ensure indices are within bounds due to halved ring segments
				if (index2 > index1 && index4 > index3) {
					// Flip the face by reversing the order of the indices
					meshData->mIndices.push_back(index1);
					meshData->mIndices.push_back(index3);
					meshData->mIndices.push_back(index2);
					
					meshData->mIndices.push_back(index3);
					meshData->mIndices.push_back(index4);
					meshData->mIndices.push_back(index2);
				}
			}
		}
	}
	
	// Set colors for visualization
	meshDataZ.mColor = glm::vec3(0.0, 0.0, 1.0);
	meshDataX.mColor = glm::vec3(1.0, 0.0, 0.0);
	meshDataY.mColor = glm::vec3(0.0, 1.0, 0.0);
	
	std::vector<std::unique_ptr<GizmoMesh::MeshData>> meshDataVector;
	meshDataVector.push_back(std::move(meshDataPtrZ));
	meshDataVector.push_back(std::move(meshDataPtrX));
	meshDataVector.push_back(std::move(meshDataPtrY));
	
	return meshDataVector;
}
