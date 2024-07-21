// HierarchyPanel.cpp
#include "ui/HierarchyPanel.hpp"

#include <nanogui/layout.h>
#include <nanogui/treeviewitem.h>

#include "actors/Actor.hpp"  // Include the Actor header

HierarchyPanel::HierarchyPanel(nanogui::Widget &parent) : Panel(parent, "Hierarchy") {
    set_position(nanogui::Vector2i(0, 0));
    set_layout(new nanogui::GroupLayout());

    mScrollPanel = new nanogui::VScrollPanel(this);

    mScrollPanel->set_fixed_size({0, 12 * 25});

    mTreeView = new nanogui::TreeView(mScrollPanel);
    mTreeView->set_layout(new nanogui::GridLayout(nanogui::Orientation::Horizontal, 1 /*columns */,
                                                  nanogui::Alignment::Fill, 3, 3));
}

bool HierarchyPanel::mouse_drag_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel,
                                      int button, int modifiers) {
    // Disable dragging
    return mScrollPanel->mouse_drag_event(p, rel, button, modifiers);
}

void HierarchyPanel::set_actors(const std::vector<Actor *> &actors) {
    mTreeView->clear();
    for (Actor *actor : actors) {
        populate_tree(actor);
    }
}

void HierarchyPanel::populate_tree(Actor *actor, nanogui::TreeViewItem *parent_node) {
    // Correctly reference the actor's name
    nanogui::TreeViewItem *node =
        parent_node ? parent_node->add_node("actor->name()") : mTreeView->add_node("actor->name()");
    // Uncomment and correctly iterate over the actor's children
    //    for (Actor *child : actor->children()) {
    //        populate_tree(child, node);
    //    }
}
