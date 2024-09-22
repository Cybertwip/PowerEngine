#include "UiCommon.hpp"

#include "actors/Actor.hpp"
#include "components/UiComponent.hpp"
#include "grok/PromptBox.hpp"
#include "ui/AnimationPanel.hpp"
#include "ui/HierarchyPanel.hpp"
#include "ui/ScenePanel.hpp"
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
	int sceneHeight = static_cast<int>(totalHeight * 0.90);
	int statusHeight = static_cast<int>(totalHeight * 0.05);
	int toolboxHeight = static_cast<int>(totalHeight * 0.05);

	sceneWrapper->set_fixed_width(totalWidth);
	sceneWrapper->set_fixed_height(totalHeight);

	auto leftWrapper = new nanogui::Window(sceneWrapper, "");
	leftWrapper->set_layout(
							new nanogui::GridLayout(nanogui::Orientation::Horizontal, 1, nanogui::Alignment::Minimum));

	leftWrapper->set_fixed_width(sceneWidth);
	leftWrapper->set_fixed_height(totalHeight);

	mToolbox = new Panel(*leftWrapper, "");
	mToolbox->set_layout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal,
												 nanogui::Alignment::Minimum, 4, 2));
	mToolbox->set_fixed_height(toolboxHeight);

    mScenePanel = new ScenePanel(*leftWrapper);

    mScenePanel->set_fixed_width(sceneWidth);
    mScenePanel->set_fixed_height(sceneHeight);

	mStatusBar = new nanogui::Widget(leftWrapper);

	mStatusBar->set_fixed_width(leftWrapper->fixed_width());
	mStatusBar->set_fixed_height(statusHeight);

    auto rightWrapper = new nanogui::Window(sceneWrapper, "");
    rightWrapper->set_layout(
        new nanogui::BoxLayout(nanogui::Orientation::Vertical, nanogui::Alignment::Fill));
    rightWrapper->set_fixed_width(rightWidth);

	mTransformPanel = new TransformPanel(*rightWrapper);

	mAnimationPanel = new AnimationPanel(*rightWrapper);

	mHierarchyPanel = new HierarchyPanel(*mScenePanel, *mTransformPanel, *mAnimationPanel, *rightWrapper);

//	auto promptbox = new PromptBox(*rightWrapper);
	
	mHierarchyPanel->inc_ref();
	mTransformPanel->inc_ref();
	mAnimationPanel->inc_ref();
//	promptbox->inc_ref();
	
	rightWrapper->remove_child(mHierarchyPanel);
	rightWrapper->remove_child(mTransformPanel);
	rightWrapper->remove_child(mAnimationPanel);
//	rightWrapper->remove_child(promptbox);

	rightWrapper->add_child(mHierarchyPanel); // Add HierarchyPanel first
	rightWrapper->add_child(mTransformPanel); // Add TransformPanel second
	rightWrapper->add_child(mAnimationPanel); // Add AnimationPanel third
//	rightWrapper->add_child(promptbox); // Add Grok third
}

