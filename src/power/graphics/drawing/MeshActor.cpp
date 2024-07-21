#include "graphics/drawing/MeshActor.hpp"

#include <functional>

#include "components/DrawableComponent.hpp"
#include "components/MeshComponent.hpp"
#include "components/TransformComponent.hpp"
#include "import/Fbx.hpp"

MeshActor::MeshActor(entt::registry& registry, const std::string& path,
                     SkinnedMesh::SkinnedMeshShader& meshShaderWrapper)
    : Actor(registry) {
    mModel = std::make_unique<Fbx>("models/DeepMotionBot.fbx");

    std::vector<std::reference_wrapper<SkinnedMesh>> meshComponentData;

    for (auto& meshData : mModel->GetMeshData()) {
        auto& mesh =
            mMeshes.emplace_back(std::make_unique<SkinnedMesh>(*meshData, meshShaderWrapper));
        meshComponentData.push_back(*mesh);
    }

    mMeshComponent = std::make_unique<MeshComponent>(meshComponentData);

    add_component<DrawableComponent>(*mMeshComponent);
    add_component<TransformComponent>();
}
