#pragma once

#include "ShaderManager.hpp"
#include "actors/ActorManager.hpp"
#include "components/TransformComponent.hpp"
#include "components/MetadataComponent.hpp"
#include "graphics/drawing/GizmoMesh.hpp"
#include <nanogui/opengl.h>
#include <vector>
#include <memory>

class ScaleGizmo {
public:
	ScaleGizmo(ShaderManager& shaderManager);
	void select(int gizmoId);
	void hover(int gizmoId);
	void transform(std::optional<std::reference_wrapper<Actor>> actor, float px, float py);

	void draw_content(std::optional<std::reference_wrapper<Actor>> actor, const nanogui::Matrix4f& model, const nanogui::Matrix4f& view, const nanogui::Matrix4f& projection);
	
private:
	int mGizmoId;
	int mHoverGizmoId;
	
	float mScaleFactor;

	std::vector<std::unique_ptr<GizmoMesh::MeshData>> mMeshData;
	
	std::unique_ptr<GizmoMesh::GizmoMeshShader> mGizmoShader;
	std::vector<std::unique_ptr<GizmoMesh>> mGizmo;
	std::vector<nanogui::Matrix4f> mTransforms;
	std::vector<MetadataComponent> mMetadataComponents;
	
	std::vector<std::unique_ptr<GizmoMesh::MeshData>> create_axis_mesh_data();
	std::unique_ptr<GizmoMesh::MeshData> create_axis_plane_mesh_data(const glm::vec3& color);
};

