#include "RenderCommon.hpp"

#include <nanogui/vector.h>

#include <Canvas.hpp>
#include <ShaderManager.hpp>

RenderCommon::RenderCommon(nanogui::Widget& parent, RenderManager& renderManager,
                           RenderSettings& renderSettings) {
    mCanvas =
        std::make_unique<Canvas>(&parent, renderManager, nanogui::Color{70, 130, 180, 255},
                                 nanogui::Vector2i{renderSettings.mCanvasWidth, renderSettings.mCanvasHeight});
    mShaderManager = std::make_unique<ShaderManager>(*mCanvas);
}
