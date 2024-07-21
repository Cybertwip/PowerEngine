// HierarchyPanel.hpp
#pragma once

#include "Panel.hpp"

#include <nanogui/treeview.h>
#include <nanogui/vscrollpanel.h>
#include <vector>

class Actor; // Forward declaration

class HierarchyPanel : public Panel {
public:
    HierarchyPanel(nanogui::Widget &parent);

    void set_actors(const std::vector<Actor*> &actors);

private:
    bool mouse_drag_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel,
                                  int button, int modifiers) override;
    
private:
    nanogui::VScrollPanel *mScrollPanel;
    nanogui::TreeView *mTreeView;
    void populate_tree(Actor *actor, nanogui::TreeViewItem *parentNode = nullptr);
};
