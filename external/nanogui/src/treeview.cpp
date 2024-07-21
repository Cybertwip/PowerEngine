#include <nanogui/treeview.h>
#include <nanogui/treeviewitem.h>
#include <nanogui/opengl.h>

NAMESPACE_BEGIN(nanogui)

TreeView::TreeView(Widget *parent)
    : Widget(parent) {
}

void TreeView::draw(NVGcontext *ctx) {
    Widget::draw(ctx);
}

void TreeView::add_item(TreeViewItem *item) {
    m_items.push_back(item);
}

TreeViewItem* TreeView::add_node(const std::string &caption) {
    TreeViewItem *node = new TreeViewItem(this, caption);
    add_item(node);
    return node;
}

void TreeView::clear() {
    for (auto item : m_items) {
        remove_child(item);
        delete item;
    }
    m_items.clear();
}


NAMESPACE_END(nanogui)
