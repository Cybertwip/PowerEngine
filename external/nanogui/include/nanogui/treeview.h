#pragma once

#include <nanogui/nanogui.h>
#include <functional>
#include <vector>

NAMESPACE_BEGIN(nanogui)

class TreeViewItem;

class TreeView : public Widget {
public:
    TreeView(Widget *parent);

    virtual void draw(NVGcontext *ctx) override;

    void add_item(TreeViewItem *item);
    TreeViewItem* add_node(const std::string &caption, std::function<void()> callback);
    void clear();

private:
    std::vector<TreeViewItem *> m_items;
};

NAMESPACE_END(nanogui)

