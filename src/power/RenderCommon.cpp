#include "RenderCommon.hpp"

#include <nanogui/vector.h>

#include <Canvas.hpp>
#include <ShaderManager.hpp>

RenderCommon::RenderCommon(nanogui::Widget& parent, RenderManager& renderManager,
                           PowerEngineSettings settings) {
    mCanvas =
        std::make_unique<Canvas>(&parent, renderManager, nanogui::Color{70, 130, 180, 255},
                                 nanogui::Vector2i{settings.mCanvasWidth, settings.mCanvasHeight});
    mShaderManager = std::make_unique<ShaderManager>(*mCanvas);
}
