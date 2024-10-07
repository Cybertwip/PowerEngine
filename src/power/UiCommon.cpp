#include "UiCommon.hpp"

#include "actors/Actor.hpp"
#include "components/UiComponent.hpp"
#include "grok/PromptBox.hpp"
#include "ui/AnimationPanel.hpp"
#include "ui/HierarchyPanel.hpp"
#include "ui/ScenePanel.hpp"
#include "ui/SceneTimeBar.hpp"
#include "ui/TransformPanel.hpp"
#include "ui/UiManager.hpp"

UiCommon::UiCommon(nanogui::Widget&, ActorManager& actorManager, AnimationTimeProvider& animationTimeProvider) : nanogui::Widget(parent), mActorManager(actorManager), mAnimationTimeProvider(animationTimeProvider) {
}

void UiCommon::initialize() {
	mMainWrapper = std::make_shared<nanogui::Window>(*this, "");
		
	mMainWrapper->set_layout(
							 std::make_unique<nanogui::GridLayout>(nanogui::Orientation::Vertical, 2, nanogui::Alignment::Fill, 0, 0));
	mSceneWrapper = std::make_shared<nanogui::Window>(mMainWrapper, "");
	
	mSceneWrapper->set_layout(std::make_unique<nanogui::GridLayout>(nanogui::Orientation::Horizontal, 2,
																	nanogui::Alignment::Fill, 0, 0));
	
	int totalWidth = parent()->size().x();
	int sceneWidth = static_cast<int>(totalWidth * 0.80f);
	int rightWidth = totalWidth - sceneWidth;
	
	int totalHeight = parent()->size().y();
	int sceneHeight = static_cast<int>(totalHeight * 0.90);
	int statusHeight = static_cast<int>(totalHeight * 0.05);
	int toolboxHeight = static_cast<int>(totalHeight * 0.05);
	
	mSceneWrapper->set_fixed_width(totalWidth);
	mSceneWrapper->set_fixed_height(totalHeight);
	
	mLeftWrapper = std::make_shared<nanogui::Window>(mSceneWrapper, "");
		
	mLeftWrapper->set_layout(
							 std::make_shared<nanogui::GridLayout>(nanogui::Orientation::Horizontal, 1, nanogui::Alignment::Minimum));
	
	mLeftWrapper->set_fixed_width(sceneWidth);
	mLeftWrapper->set_fixed_height(totalHeight);
	
	mToolbox = std::make_shared<Panel>(mLeftWrapper, "");
	
	mToolbox->set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Horizontal,
															  nanogui::Alignment::Minimum, 4, 2));
	mToolbox->set_fixed_height(toolboxHeight);
	
	mScenePanel = std::make_shared<ScenePanel>(mLeftWrapper);
	
	mScenePanel->set_fixed_width(sceneWidth);
	mScenePanel->set_fixed_height(sceneHeight);
	
	mStatusBar = std::make_shared<nanogui::Widget>(mLeftWrapper);
	
	mStatusBar->set_fixed_width(mLeftWrapper->fixed_width());
	mStatusBar->set_fixed_height(statusHeight);
	
	mRightWrapper = std::make_shared<nanogui::Window>(mSceneWrapper, "");
	
	mRightWrapper->set_layout(
							  std::make_shared<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Fill));
	mRightWrapper->set_fixed_width(rightWidth);
	
	mTransformPanel = std::make_shared<TransformPanel>(mRightWrapper);
	
	mAnimationPanel = std::make_shared<AnimationPanel>(mRightWrapper);
	
	mHierarchyPanel = std::make_shared<HierarchyPanel>(mScenePanel, mTransformPanel, mAnimationPanel, mActorManager, mRightWrapper);
	
	//	auto promptbox = new PromptBox(*rightWrapper);
	//	promptbox->inc_ref();
	
	mRightWrapper->remove_child(*mHierarchyPanel);
	mRightWrapper->remove_child(*mTransformPanel);
	mRightWrapper->remove_child(*mAnimationPanel);
	//	rightWrapper->remove_child(promptbox);
	
	mRightWrapper->add_child(*mHierarchyPanel); // Add HierarchyPanel first
	mRightWrapper->add_child(*mTransformPanel); // Add TransformPanel second
	mRightWrapper->add_child(*mAnimationPanel); // Add AnimationPanel third
	//	rightWrapper->add_child(promptbox); // Add Grok third
	
	// Initialize the scene time bar
	mSceneTimeBar = std::make_shared<SceneTimeBar>(mScenePanel, mActorManager, mAnimationTimeProvider, mHierarchyPanel,  mScenePanel->fixed_width(), mScenePanel->fixed_height() * 0.25f);
}

