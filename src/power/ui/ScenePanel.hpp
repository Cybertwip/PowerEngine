#pragma once

#include "Canvas.hpp"

#include <nanogui/window.h>

class ScenePanel : public nanogui::Window {
public:
    ScenePanel(nanogui::Widget *parent);
};
