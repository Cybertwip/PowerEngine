#pragma once

#include "actors/Actor.hpp"
#include "components/MetadataComponent.hpp"
#include "components/TransformComponent.hpp"
#include "graphics/drawing/GizmoMesh.hpp"
#include <memory>
#include <vector>
#include <optional>

class ShaderManager;

class RotationGizmo {
public:
	RotationGizmo(ShaderManager& shaderManager);
	
	void select(int gizmoId);
	void hover(int gizmoId);
	void transform(std::optional<std::reference_wrapper<Actor>> actor, float angle);
	void draw_content(std::optional<std::reference_wrapper<Actor>> actor,
					  const nanogui::Matrix4f& model,
					  const nanogui::Matrix4f& view,
					  const nanogui::Matrix4f& projection);
	
private:
	std::vector<std::unique_ptr<GizmoMesh>> mRotationGizmo;
	std::vector<nanogui::Matrix4f> mRotationTransforms;
	std::vector<std::unique_ptr<GizmoMesh::MeshData>> mMeshData;
	std::vector<MetadataComponent> mMetadataComponents;
	
	std::unique_ptr<GizmoMesh::GizmoMeshShader> mGizmoShader;
	int mGizmoId;
	int mHoverGizmoId;
	float mScaleFactor;
	
	std::vector<std::unique_ptr<GizmoMesh::MeshData>> create_axis_rotation_mesh_data();
};
