#include <nanogui/treeview.h>
#include <nanogui/treeviewitem.h>
#include <nanogui/opengl.h>

NAMESPACE_BEGIN(nanogui)

TreeView::TreeView(Widget& parent, Screen& screen, Theme& theme)
    : Widget(parent, screen, theme), m_selected_item(nullptr) {
}

void TreeView::draw(NVGcontext *ctx) {
    Widget::draw(ctx);
}

void TreeView::add_item(std::shared_ptr<TreeViewItem> item) {
    m_items.push_back(item);
}

std::shared_ptr<TreeViewItem> TreeView::add_node(const std::string &caption, std::function<void()> callback) {
	auto node = std::make_shared<TreeViewItem>(*this, dynamic_cast<TreeView>(*this), caption, callback);
    add_item(node);
    return node;
}

void TreeView::clear() {
    for (auto item : m_items) {
        remove_child(item);
    }
    m_items.clear();
	
	m_selected_item = nullptr;
}

void TreeView::set_selected(std::shared_ptr<TreeViewItem> item) {
    if (m_selected_item) {
        m_selected_item->set_selected(false);
    }
    
	if (item) {
		m_selected_item = item;
		
		m_selected_item->set_selected(true);
	}
}

NAMESPACE_END(nanogui)
