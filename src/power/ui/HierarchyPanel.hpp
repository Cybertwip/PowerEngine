// HierarchyPanel.hpp
#pragma once

#include "Panel.hpp"

#include "actors/IActorSelectedRegistry.hpp"

#include <nanogui/treeview.h>
#include <nanogui/vscrollpanel.h>
#include <vector>

class IActorSelectedCallback;

class Actor;
class ActorManager;
class AnimationPanel;
class ScenePanel;
class TransformPanel;

class HierarchyPanel : public IActorSelectedRegistry, public IActorVisualManager, public Panel {
public:
	HierarchyPanel(ScenePanel& scenePanel, TransformPanel& transformPanel, AnimationPanel& animationPanel, ActorManager& actorManager, std::shared_ptr<nanogui::Widget> parent);
	
	~HierarchyPanel() = default;

    void add_actors(const std::vector<std::reference_wrapper<Actor>> &actors) override;

	void add_actor(std::reference_wrapper<Actor> actor) override;
	
	void remove_actor(std::reference_wrapper<Actor> actor) override;
	
	void fire_actor_selected_event(std::optional<std::reference_wrapper<Actor>> actor) override;
	void RegisterOnActorSelectedCallback(IActorSelectedCallback& callback) override;
	
	void UnregisterOnActorSelectedCallback(IActorSelectedCallback& callback) override;
	
private:
	std::vector<std::reference_wrapper<IActorSelectedCallback>> mActorSelectedCallbacks;

private:
    bool mouse_drag_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel,
                                  int button, int modifiers) override;
    
	void OnActorSelected(std::optional<std::reference_wrapper<Actor>> actor);

private:
	TransformPanel& mTransformPanel;
	AnimationPanel& mAnimationPanel;
    nanogui::VScrollPanel *mScrollPanel;
	ActorManager& mActorManager;
	
    nanogui::TreeView *mTreeView;
    void populate_tree(Actor& actor, nanogui::TreeViewItem *parentNode = nullptr);
};
