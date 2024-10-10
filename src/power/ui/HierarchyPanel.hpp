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
	HierarchyPanel(nanogui::Widget& parent, nanogui::Screen& screen, std::shared_ptr<ScenePanel> scenePanel, std::shared_ptr<TransformPanel> transformPanel, std::shared_ptr<AnimationPanel> animationPanel, ActorManager& actorManager);
	
	~HierarchyPanel() = default;

    void add_actors(const std::vector<std::reference_wrapper<Actor>> &actors) override;

	void add_actor(std::reference_wrapper<Actor> actor) override;
	
	void remove_actor(std::reference_wrapper<Actor> actor) override;

	void remove_actors(const std::vector<std::reference_wrapper<Actor>>& actors) override;

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
	std::shared_ptr<TransformPanel> mTransformPanel;
	std::shared_ptr<AnimationPanel> mAnimationPanel;
	std::shared_ptr<nanogui::VScrollPanel> mScrollPanel;
	ActorManager& mActorManager;
	
	std::shared_ptr<nanogui::TreeView> mTreeView;
    void populate_tree(Actor& actor, std::shared_ptr<nanogui::TreeViewItem> parentNode = nullptr);
};
