#include "UiCommon.hpp"

#include "ui/HierarchyPanel.hpp"
#include "ui/ScenePanel.hpp"
#include "ui/TransformPanel.hpp"

UiCommon::UiCommon(nanogui::Widget& parent) {
    mScenePanel = new ScenePanel(parent);
    auto wrapper = new nanogui::Window(&parent, "");
    wrapper->set_layout(
        new nanogui::BoxLayout(nanogui::Orientation::Vertical, nanogui::Alignment::Fill));

    mHierarchyPanel = new HierarchyPanel(*wrapper);
    mTransformPanel = new TransformPanel(*wrapper);
}
