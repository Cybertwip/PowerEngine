#include "UiCommon.hpp"

#include "actors/Actor.hpp"
#include "components/UiComponent.hpp"
#include "ui/HierarchyPanel.hpp"
#include "ui/ScenePanel.hpp"
#include "ui/StatusBarPanel.hpp"
#include "ui/TransformPanel.hpp"
#include "ui/UiManager.hpp"

UiCommon::UiCommon(nanogui::Widget& parent, ActorManager& actorManager) {
    auto mainWrapper = new nanogui::Window(&parent, "");
    mainWrapper->set_layout(
        new nanogui::GridLayout(nanogui::Orientation::Vertical, 2, nanogui::Alignment::Fill, 0, 0));

    auto sceneWrapper = new nanogui::Window(mainWrapper, "");
    sceneWrapper->set_layout(new nanogui::GridLayout(nanogui::Orientation::Horizontal, 2,
                                                     nanogui::Alignment::Fill, 0, 0));

    int totalWidth = parent.size().x();
    int sceneWidth = static_cast<int>(totalWidth * 0.80f);
    int rightWidth = totalWidth - sceneWidth;

    int totalHeight = parent.size().y();
    int sceneHeight = static_cast<int>(totalHeight * 0.95f);
    int statusHeight = totalHeight - sceneHeight;

    sceneWrapper->set_fixed_width(totalWidth);

    mScenePanel = new ScenePanel(*sceneWrapper);
    mScenePanel->set_fixed_width(sceneWidth);
    mScenePanel->set_fixed_height(sceneHeight);

    auto rightWrapper = new nanogui::Window(sceneWrapper, "");
    rightWrapper->set_layout(
        new nanogui::BoxLayout(nanogui::Orientation::Vertical, nanogui::Alignment::Fill));
    rightWrapper->set_fixed_width(rightWidth);


	mTransformPanel = new TransformPanel(*rightWrapper);

	mHierarchyPanel = new HierarchyPanel(*mScenePanel, *mTransformPanel, *rightWrapper);

	mHierarchyPanel->inc_ref();
	mTransformPanel->inc_ref();
	
	rightWrapper->remove_child(mHierarchyPanel);
	rightWrapper->remove_child(mTransformPanel);
	
	rightWrapper->add_child(mHierarchyPanel); // Add HierarchyPanel first
	rightWrapper->add_child(mTransformPanel); // Add TransformPanel second

    mStatusBarPanel = new StatusBarPanel(*mainWrapper);

    mStatusBarPanel->set_fixed_height(statusHeight);
}

void UiCommon::attach_actors(const std::vector<std::reference_wrapper<Actor>>& actors) {
    mHierarchyPanel->set_actors(actors);
}

void UiCommon::update() { mTransformPanel->update(); }
