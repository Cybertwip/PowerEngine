#pragma once

#include "Panel.hpp"

#include <vector>
#include <optional>
#include <functional>
#include <memory>

class Actor;
class ActorManager;
class HierarchyPanel;

// Forward-declare nanogui classes
namespace nanogui {
class Button; // Forward-declare Button
}

class HierarchyToolPanel : public Panel {
public:
	HierarchyToolPanel(nanogui::Widget& parent, ActorManager& actorManager, HierarchyPanel& hierarchyPanel);
	
	~HierarchyToolPanel() = default;
		
private:
	ActorManager& mActorManager;
	HierarchyPanel& mHierarchyPanel;
	
	std::shared_ptr<nanogui::Button> mRemoveActorButton;
	std::shared_ptr<nanogui::Button> mAddActorButton;

};
