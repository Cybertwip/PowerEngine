#include "UiCommon.hpp"

#include "ui/ScenePanel.hpp"
#include "ui/TransformPanel.hpp"

UiCommon::UiCommon(nanogui::Widget& parent)
    : mScenePanel(new ScenePanel(parent)), mTransformPanel(new TransformPanel(parent)) {}
