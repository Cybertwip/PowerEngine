#include "RenderCommon.hpp"

#include <nanogui/vector.h>

#include "Canvas.hpp"
#include "MeshActorLoader.hpp"
#include "ShaderManager.hpp"
#include "actors/Actor.hpp"
#include "actors/ActorManager.hpp"
#include "gizmo/GizmoManager.hpp"

RenderCommon::RenderCommon(nanogui::Widget& parent, entt::registry& registry,
                           ActorManager& actorManager) {
    mCanvas = std::make_unique<Canvas>(&parent, nanogui::Color{70, 130, 180, 255});
    mShaderManager = std::make_unique<ShaderManager>(*mCanvas);

    mMeshActorLoader = std::make_unique<MeshActorLoader>(registry, *mShaderManager);
    mGizmoManager = std::make_unique<GizmoManager>(*mShaderManager, actorManager);

    mCanvas->register_draw_callback([this, &actorManager]() {
        // actorManager.draw();
        mGizmoManager->draw();
    });
}
