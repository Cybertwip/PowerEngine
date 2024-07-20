#pragma once

#include "Canvas.hpp"

#include <nanogui/widget.h>

class RenderManager;

class ScenePanel : public nanogui::Widget {
public:
    ScenePanel(nanogui::Widget *parent, RenderManager& renderManager);
    
    nanogui::RenderPass& AcquireRenderPass() const {
        return *mCanvas->render_pass();
    }

    virtual ~ScenePanel() {}

private:
    Canvas *mCanvas;
};
