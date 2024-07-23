#include "actors/GizmoActor.hpp"

#include <filesystem>
#include <functional>

#include "components/DrawableComponent.hpp"
#include "components/GizmoComponent.hpp"
#include "components/MetadataComponent.hpp"
#include "components/TransformComponent.hpp"
#include "import/Fbx.hpp"

GizmoActor::GizmoActor(entt::registry& registry, const std::string& path,
                       GizmoMesh::GizmoMeshShader& gizmoShaderWrapper)
    : Actor(registry) {
    mGizmoComponent = std::make_unique<GizmoComponent>();

    add_component<DrawableComponent>(*mGizmoComponent);
    add_component<TransformComponent>();
    add_component<MetadataComponent>(std::filesystem::path(path).stem().string());
}
