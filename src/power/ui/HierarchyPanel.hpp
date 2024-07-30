// HierarchyPanel.hpp
#pragma once

#include "Panel.hpp"

#include "actors/IActorSelectedRegistry.hpp"

#include <nanogui/treeview.h>
#include <nanogui/vscrollpanel.h>
#include <vector>

class IActorSelectedCallback;

class Actor; // Forward declaration

class HierarchyPanel : public IActorSelectedRegistry, public Panel {
public:
    HierarchyPanel(nanogui::Widget &parent);

    void set_actors(const std::vector<std::reference_wrapper<Actor>> &actors);

	void RegisterOnActorSelectedCallback(IActorSelectedCallback& callback) override;
	
	void UnregisterOnActorSelectedCallback(IActorSelectedCallback& callback) override;
	
private:
	std::vector<std::reference_wrapper<IActorSelectedCallback>> actorSelectedCallbacks;

private:
    bool mouse_drag_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel,
                                  int button, int modifiers) override;
    
	void OnActorSelected(Actor& actor);

private:
    nanogui::VScrollPanel *mScrollPanel;
    nanogui::TreeView *mTreeView;
    void populate_tree(Actor& actor, nanogui::TreeViewItem *parentNode = nullptr);
};
