#include "GizmoManager.hpp"

#include <cmath>

#include "ShaderManager.hpp"
#include "actors/ActorManager.hpp"
#include "components/TransformComponent.hpp"

std::vector<std::unique_ptr<GizmoMesh::MeshData>> create_axis_mesh_data() {
    const int numSegments = 20;
    const float radius = 0.02f;
    const float length = 1.0f;
    const float coneHeight = 0.1f;
    const float coneBaseRadius = 0.05f;

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

        meshDataZ.mVertices.push_back(
            std::make_unique<GizmoMesh::Vertex>(glm::vec3({x, length, z})));
        meshDataZ.mVertices.push_back(
            std::make_unique<GizmoMesh::Vertex>(glm::vec3({0.0f, length + coneHeight, 0.0f})));
    }

    // Generate cylinder for Z axis
    for (int i = 0; i <= numSegments; ++i) {
        float theta = 2.0f * M_PI * float(i) / float(numSegments);
        float x = radius * cosf(theta);
        float y = radius * sinf(theta);

        meshDataZ.mVertices.push_back(std::make_unique<GizmoMesh::Vertex>(glm::vec3({x, y, 0.0f})));
        meshDataZ.mVertices.push_back(
            std::make_unique<GizmoMesh::Vertex>(glm::vec3({x, y, length})));
    }

    // Generate cone for Z axis arrowhead
    for (int i = 0; i <= numSegments; ++i) {
        float theta = 2.0f * M_PI * float(i) / float(numSegments);
        float x = coneBaseRadius * cosf(theta);
        float y = coneBaseRadius * sinf(theta);

        meshDataZ.mVertices.push_back(
            std::make_unique<GizmoMesh::Vertex>(glm::vec3({x, y, length})));
        meshDataZ.mVertices.push_back(
            std::make_unique<GizmoMesh::Vertex>(glm::vec3({0.0f, 0.0f, length + coneHeight})));
    }

    std::vector<std::unique_ptr<GizmoMesh::MeshData>> meshDataVector{
        std::move(meshDataPtrX), std::move(meshDataPtrY), std::move(meshDataPtrZ)};

    // Indices for X, Y, and Z axes
    for (unsigned int i = 0; i < meshDataVector.size(); ++i) {
        auto& meshData = *meshDataVector[i];
        unsigned int baseIndex = 0;
        for (int j = 0; j < 3; ++j) {
            baseIndex = j * (numSegments + 1) * 4;
            for (int i = 0; i < numSegments; ++i) {
                meshData.mIndices.push_back(baseIndex + i * 2);
                meshData.mIndices.push_back(baseIndex + i * 2 + 1);
                meshData.mIndices.push_back(baseIndex + i * 2 + 2);

                meshData.mIndices.push_back(baseIndex + i * 2 + 1);
                meshData.mIndices.push_back(baseIndex + i * 2 + 3);
                meshData.mIndices.push_back(baseIndex + i * 2 + 2);
            }
            baseIndex += (numSegments + 1) * 2;
            for (int i = 0; i < numSegments; ++i) {
                meshData.mIndices.push_back(baseIndex + i * 2);
                meshData.mIndices.push_back(baseIndex + i * 2 + 1);
                meshData.mIndices.push_back(baseIndex + i * 2 + 2);

                meshData.mIndices.push_back(baseIndex + i * 2 + 1);
                meshData.mIndices.push_back(baseIndex + i * 2 + 3);
                meshData.mIndices.push_back(baseIndex + i * 2 + 2);
            }
        }
    }

    meshDataVector[0]->mColor = glm::vec3(1.0, 0.0, 0.0);
    meshDataVector[1]->mColor = glm::vec3(0.0, 1.0, 0.0);
    meshDataVector[2]->mColor = glm::vec3(0.0, 0.0, 1.0);

    return std::move(meshDataVector);
}

GizmoManager::GizmoManager(ShaderManager& shaderManager, ActorManager& actorManager)
    : mShaderManager(shaderManager), mActorManager(actorManager) {
    mGizmoShader = std::make_unique<GizmoMesh::GizmoMeshShader>(mShaderManager);

    mTranslationGizmoMeshData = std::move(create_axis_mesh_data());

    mTranslationGizmo.push_back(
        std::make_unique<GizmoMesh>(*mTranslationGizmoMeshData[0], *mGizmoShader));
    mTranslationGizmo.push_back(
        std::make_unique<GizmoMesh>(*mTranslationGizmoMeshData[1], *mGizmoShader));
    mTranslationGizmo.push_back(
        std::make_unique<GizmoMesh>(*mTranslationGizmoMeshData[2], *mGizmoShader));
}

void GizmoManager::draw() { mActorManager.visit(*this); }

void GizmoManager::draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view,
                                const nanogui::Matrix4f& projection) {
    for (auto& axis : mTranslationGizmo) {
        axis->draw_content(model, view, projection);
    }
}
