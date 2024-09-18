#include "GizmoMesh.hpp"

#include <GLFW/glfw3.h>
#include <nanogui/renderpass.h>
#include <nanogui/texture.h>

#include <cmath>

#include "CameraManager.hpp"
#include "Canvas.hpp"
#include "ShaderManager.hpp"
#include "graphics/shading/ShaderWrapper.hpp"
#include "import/Fbx.hpp"

GizmoMesh::Vertex::Vertex() {}

GizmoMesh::Vertex::Vertex(const glm::vec3& pos) : mPosition(pos) {}

void GizmoMesh::Vertex::set_position(const glm::vec3& vec) { mPosition = vec; }

glm::vec3 GizmoMesh::Vertex::get_position() const { return mPosition; }

GizmoMesh::GizmoMeshShader::GizmoMeshShader(ShaderManager& shaderManager)
    : ShaderWrapper(*shaderManager.get_shader("mesh")) {}

void GizmoMesh::GizmoMeshShader::upload_vertex_data(
    const std::vector<std::unique_ptr<Vertex>>& vertexData) {
    std::vector<float> positions;
    for (const auto& vertexPointer : vertexData) {
        auto& vertex = *vertexPointer;
        positions.insert(positions.end(), {vertex.get_position().x, vertex.get_position().y,
                                           vertex.get_position().z});
    }

    mShader.set_buffer("aPosition", nanogui::VariableType::Float32, {vertexData.size(), 3},
                       positions.data());
}

GizmoMesh::GizmoMesh(MeshData& meshData, GizmoMeshShader& shader)
    : mMeshData(meshData), mShader(shader) {
}

void GizmoMesh::draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view,
                             const nanogui::Matrix4f& projection) {
	mShader.set_buffer("indices", nanogui::VariableType::UInt32, {mMeshData.mIndices.size()},
					   mMeshData.mIndices.data());
	mShader.upload_vertex_data(mMeshData.mVertices);

    mShader.set_uniform("u_color",
                        nanogui::Vector4f(mMeshData.mColor.x, mMeshData.mColor.y, mMeshData.mColor.z,
						    mMeshData.mColor.w));
    mShader.set_uniform("aModel", model);
    mShader.set_uniform("aView", view);
    mShader.set_uniform("aProjection", projection);

    mShader.begin();
    mShader.draw_array(nanogui::Shader::PrimitiveType::Triangle, 0, mMeshData.mIndices.size(), true);
    mShader.end();
}
