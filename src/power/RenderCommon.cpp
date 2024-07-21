#include "RenderCommon.hpp"

#include <nanogui/vector.h>

#include "Canvas.hpp"
#include "MeshActorLoader.hpp"
#include "ShaderManager.hpp"

#include "actors/Actor.hpp"
#include "actors/ActorManager.hpp"

RenderCommon::RenderCommon(nanogui::Widget& parent, entt::registry& registry, ActorManager& actorManager,
                           RenderSettings& renderSettings) {
    mCanvas = std::make_unique<Canvas>(
        &parent, actorManager, nanogui::Color{70, 130, 180, 255},
        nanogui::Vector2i{renderSettings.mCanvasWidth, renderSettings.mCanvasHeight});
    mShaderManager = std::make_unique<ShaderManager>(*mCanvas);
    mMeshActorLoader =
        std::make_unique<MeshActorLoader>(registry, *mShaderManager);
}
