#include "ui/HierarchyToolPanel.hpp"

#include <nanogui/button.h>
#include <nanogui/layout.h>

#include "CameraManager.hpp"

#include "actors/Actor.hpp"
#include "actors/ActorManager.hpp"
#include "execution/BlueprintManager.hpp"
#include "ui/UiManager.hpp"


HierarchyToolPanel::HierarchyToolPanel(nanogui::Widget& parent, ActorManager& actorManager, HierarchyPanel& hierarchyPanel)
: Panel(parent, "")
, mActorManager(actorManager)
, mHierarchyPanel(hierarchyPanel) {
	set_position(nanogui::Vector2i(0, 0));
	set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Horizontal, nanogui::Alignment::Fill));
	
	mRemoveActorButton = std::make_shared<nanogui::Button>(*this, "-");
	mAddActorButton = std::make_shared<nanogui::Button>(*this, "+");

	
	mRemoveActorButton->set_callback([this](){
		mHierarchyPanel.remove_selected_actor();
	});
	
	mAddActorButton->set_callback([this](){
		auto& _ = mActorManager.create_actor();
	});
}

