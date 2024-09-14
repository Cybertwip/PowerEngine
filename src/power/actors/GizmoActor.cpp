#include "actors/GizmoActor.hpp"

#include <filesystem>
#include <functional>

#include "components/DrawableComponent.hpp"
#include "components/GizmoComponent.hpp"
#include "components/MetadataComponent.hpp"
#include "components/TransformComponent.hpp"

GizmoActor::GizmoActor(entt::registry& registry, const std::string& path,
                       GizmoMesh::GizmoMeshShader& gizmoShaderWrapper)
    : Actor(registry) {

    add_component<DrawableComponent>(std::make_unique<GizmoComponent>());
    add_component<TransformComponent>();
    add_component<MetadataComponent>(identifier(), std::filesystem::path(path).stem().string());
}
