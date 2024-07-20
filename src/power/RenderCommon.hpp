#pragma once

#include <memory>

namespace nanogui{
class Widget;
}

class Canvas;
class RenderManager;
class ShaderManager;

struct RenderSettings {
    int mCanvasWidth;
    int mCanvasHeight;
};

class RenderCommon {
public:
    RenderCommon(nanogui::Widget& parent, RenderManager& renderManager, RenderSettings& settings&;
    Canvas& canvas() {
        return *mCanvas;
    }

    ShaderManager& shader_manager() {
        return *mShaderManager;
    }

private:
    std::unique_ptr<Canvas> mCanvas;
    std::unique_ptr<ShaderManager> mShaderManager;
};

