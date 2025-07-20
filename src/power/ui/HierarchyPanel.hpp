#pragma once

#include "Panel.hpp"

#include "actors/IActorSelectedRegistry.hpp"
#include "platform/ContextMenu.hpp"

#include <nanogui/treeview.h>
#include <nanogui/vscrollpanel.h>
#include <nanogui/window.h>
#include <vector>
#include <optional>
#include <functional>
#include <memory>


class IActorSelectedCallback;

class Actor;
class ActorManager;
class AnimationPanel;
class CameraPanel;
class HierarchyToolPanel;
class ScenePanel;
class TransformPanel;

// Forward-declare nanogui classes
namespace nanogui {
class Popup;
class Button; // Forward-declare Button
}

class HierarchyPanel : public IActorSelectedRegistry, public IActorVisualManager, public Panel {
public:
	HierarchyPanel(nanogui::Widget& parent, std::shared_ptr<ScenePanel> scenePanel, std::shared_ptr<TransformPanel> transformPanel, std::shared_ptr<CameraPanel> cameraPanel, std::shared_ptr<AnimationPanel> animationPanel, ActorManager& actorManager);
	
	~HierarchyPanel() = default;
	
	void add_actors(const std::vector<std::reference_wrapper<Actor>> &actors) override;
	
	void add_actor(std::reference_wrapper<Actor> actor) override;
	
	void remove_actor(std::reference_wrapper<Actor> actor) override;

	void remove_actors(const std::vector<std::reference_wrapper<Actor>>& actors) override;
	
	void clear_actors();
	
	void fire_actor_selected_event(std::optional<std::reference_wrapper<Actor>> actor) override;
	void RegisterOnActorSelectedCallback(IActorSelectedCallback& callback) override;
	
	void UnregisterOnActorSelectedCallback(IActorSelectedCallback& callback) override;
	
	bool mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;
	
	void reload();
	
private:
	std::vector<std::reference_wrapper<IActorSelectedCallback>> mActorSelectedCallbacks;
	std::optional<std::reference_wrapper<Actor>> mSelectedActor;
	
private:
	bool mouse_drag_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel,
						  int button, int modifiers) override;
	
	void OnActorSelected(std::optional<std::reference_wrapper<Actor>> actor);
	
	void refresh_selected_actor();
	
private:
	void remove_selected_actor();
	
	std::unique_ptr<nanogui::Widget> mButtonContainer;
	
	std::shared_ptr<nanogui::Button> mAddActorButton;
	std::shared_ptr<nanogui::Button> mRemoveActorButton;

	
	std::shared_ptr<TransformPanel> mTransformPanel;
	std::shared_ptr<CameraPanel> mCameraPanel;
	std::shared_ptr<AnimationPanel> mAnimationPanel;
	std::shared_ptr<nanogui::VScrollPanel> mScrollPanel;
	ActorManager& mActorManager;
	
	std::shared_ptr<nanogui::TreeView> mTreeView;
	void populate_tree(Actor& actor, std::shared_ptr<nanogui::TreeViewItem> parentNode = nullptr);
	
	std::unique_ptr<ContextMenu> mContextMenu;
	
	friend class HierarchyToolPanel;
};
