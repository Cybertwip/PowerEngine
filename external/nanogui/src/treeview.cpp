#include <nanogui/treeview.h>
#include <nanogui/treeviewitem.h>
#include <nanogui/opengl.h>

NAMESPACE_BEGIN(nanogui)

TreeView::TreeView(Widget *parent)
    : Widget(parent), m_selected_item(nullptr) {
}

void TreeView::draw(NVGcontext *ctx) {
    Widget::draw(ctx);
}

void TreeView::add_item(TreeViewItem *item) {
    m_items.push_back(item);
}

TreeViewItem* TreeView::add_node(const std::string &caption, std::function<void()> callback) {
    TreeViewItem *node = new TreeViewItem(this, this, caption, callback);
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

void TreeView::set_selected(TreeViewItem* item) {
    if (m_selected_item) {
        m_selected_item->set_selected(false);
    }
    
    m_selected_item = item;
    
    m_selected_item->set_selected(true);
}

NAMESPACE_END(nanogui)
