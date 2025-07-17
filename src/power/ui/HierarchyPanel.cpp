#include "ui/HierarchyPanel.hpp"

#include "ui/AnimationPanel.hpp"
#include "ui/ScenePanel.hpp"
#include "ui/TransformPanel.hpp"

#include <nanogui/layout.h>
#include <nanogui/opengl.h>
#include <nanogui/treeviewitem.h>
#include <nanogui/popup.h>
#include <nanogui/button.h>
#include <GLFW/glfw3.h>

#include "actors/Actor.hpp"
#include "actors/ActorManager.hpp"
#include "actors/IActorSelectedCallback.hpp"
#include "components/MetadataComponent.hpp"
#include "components/UiComponent.hpp"

HierarchyPanel::HierarchyPanel(nanogui::Widget& parent, std::shared_ptr<ScenePanel> scenePanel, std::shared_ptr<TransformPanel> transformPanel, std::shared_ptr<AnimationPanel> animationPanel, ActorManager& actorManager) : Panel(parent, "Actors"), mTransformPanel(transformPanel), mAnimationPanel(animationPanel), mActorManager(actorManager) {
	set_position(nanogui::Vector2i(0, 0));
	set_layout(std::make_unique<nanogui::GroupLayout>());
	
	mScrollPanel = std::make_shared<nanogui::VScrollPanel>(*this);
	
	mScrollPanel->set_fixed_size({0, 12 * 25});
	
	mTreeView = std::make_shared<nanogui::TreeView>(*mScrollPanel);
	mTreeView->set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Fill));
	
	mContextMenu = std::make_unique<ContextMenu>();
	
	// Populate it with items dynamically.
	mContextMenu->addItem("Action from C++ function", []() {
		std::cout << "Callback executed from a menu created in a C++ function!" << std::endl;
	});
	mContextMenu->addItem("Another action", []() {
		std::cout << "This is the second callback." << std::endl;
	});

}


bool HierarchyPanel::mouse_drag_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel,
									    int button, int modifiers) {
	// Disable dragging
	return mScrollPanel->mouse_drag_event(p, rel, button, modifiers);
}

bool HierarchyPanel::mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) {
	if (Panel::mouse_button_event(p, button, down, modifiers)) {
		return true;
	}
	
	if (button == GLFW_MOUSE_BUTTON_RIGHT && !down && mSelectedActor.has_value() && contains(p)) {
		return true;
		
		
	}
	
	return false;
}


void HierarchyPanel::add_actors(const std::vector<std::reference_wrapper<Actor>> &actors) {
	for (auto &actor : actors) {
		populate_tree(actor.get());
	}
}

void HierarchyPanel::add_actor(std::reference_wrapper<Actor> actor) {
	populate_tree(actor.get());
}

void HierarchyPanel::remove_actor(std::reference_wrapper<Actor> actor) {
	
	mTreeView->clear();
	
	fire_actor_selected_event(std::nullopt);
	
	mActorManager.remove_actor(actor);
	
	auto actors = mActorManager.get_actors_with_component<UiComponent>();
	
	for (auto& actor : actors) {
		populate_tree(actor);
	}
	
}

void HierarchyPanel::remove_actors(const std::vector<std::reference_wrapper<Actor>>& actors) {
	nanogui::async([this, actors](){
		mTreeView->clear();
		
		fire_actor_selected_event(std::nullopt);
		
		mActorManager.remove_actors(actors);
		
		auto existingActors = mActorManager.get_actors_with_component<UiComponent>();
		
		for (auto& actor : existingActors) {
			populate_tree(actor);
		}
		
	});
}

void HierarchyPanel::populate_tree(Actor &actor, std::shared_ptr<nanogui::TreeViewItem> parent_node) {
	// Correctly reference the actor's name
	std::shared_ptr<nanogui::TreeViewItem> node =
	parent_node
	? parent_node->add_node(
							std::string{actor.get_component<MetadataComponent>().name()},
							[this, &actor]() {
								OnActorSelected(actor);
							})
	: mTreeView->add_node(std::string{actor.get_component<MetadataComponent>().name()},
						    [this, &actor]() {
		OnActorSelected(actor);
	});
	
	if (actor.find_component<UiComponent>()) {
		actor.remove_component<UiComponent>();
	}
	
	actor.add_component<UiComponent>([this, &actor, node]() {
		mTreeView->set_selected(node.get());
	});
	
	perform_layout(screen().nvg_context());
}

void HierarchyPanel::RegisterOnActorSelectedCallback(IActorSelectedCallback& callback) {
	mActorSelectedCallbacks.push_back(std::ref(callback));
}

void HierarchyPanel::UnregisterOnActorSelectedCallback(IActorSelectedCallback& callback) {
	auto it = std::remove_if(mActorSelectedCallbacks.begin(), mActorSelectedCallbacks.end(),
							 [&callback](std::reference_wrapper<IActorSelectedCallback>& ref) {
		return &ref.get() == &callback;
	});
	if (it != mActorSelectedCallbacks.end()) {
		mActorSelectedCallbacks.erase(it, mActorSelectedCallbacks.end());
	}
}

void HierarchyPanel::OnActorSelected(std::optional<std::reference_wrapper<Actor>> actor) {
	mSelectedActor = actor;
	
	if (actor.has_value()) {
		mTransformPanel->set_active_actor(actor);
		mAnimationPanel->set_active_actor(actor);
		for (auto& callbackRef : mActorSelectedCallbacks) {
			callbackRef.get().OnActorSelected(actor);
		}
		
	} else {
		mTreeView->set_selected(nullptr);
		mTransformPanel->set_active_actor(actor);
		mAnimationPanel->set_active_actor(actor);
	}
	
}

void HierarchyPanel::fire_actor_selected_event(std::optional<std::reference_wrapper<Actor>> actor) {
	mSelectedActor = actor;
	
	mTransformPanel->set_active_actor(actor);
	mAnimationPanel->set_active_actor(actor);
	
	for (auto& callbackRef : mActorSelectedCallbacks) {
		callbackRef.get().OnActorSelected(actor);
	}
	
	if (!actor.has_value()) {
		mTreeView->set_selected(nullptr);
	}
}


void HierarchyPanel::clear_actors() {
	nanogui::async([this](){
		mTreeView->clear();
		fire_actor_selected_event(std::nullopt);
	});
}
