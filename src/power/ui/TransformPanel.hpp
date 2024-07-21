#pragma once

#include "Panel.hpp"

#include <nanogui/textbox.h>

class TransformComponent;

class TransformPanel : public Panel {
public:
    TransformPanel(nanogui::Widget& parent);
    
    void gather_values_into(TransformComponent& transform);
    void update_values_from(TransformComponent& transform);

private:
    nanogui::IntBox<int> *mXTranslate;
    nanogui::IntBox<int> *mYTranslate;
    nanogui::IntBox<int> *mZTranslate;

    nanogui::IntBox<int> *mPitchRotate;
    nanogui::IntBox<int> *mYawRotate;
    nanogui::IntBox<int> *mRollRotate;

    nanogui::IntBox<int> *mXScale;
    nanogui::IntBox<int> *mYScale;
    nanogui::IntBox<int> *mZScale;
};
