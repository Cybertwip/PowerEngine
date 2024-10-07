#include <nanogui/treeviewitem.h>
#include <nanogui/treeview.h>
#include <nanogui/opengl.h>

NAMESPACE_BEGIN(nanogui)

TreeViewItem::TreeViewItem(std::shared_ptr<Widget> parent, std::shared_ptr<TreeView> tree, const std::string &caption, std::function<void()> callback)
: Widget(parent), m_tree(tree), m_caption(caption), m_selected(false), m_expanded(false),  m_selection_callback(callback) {}

void TreeViewItem::draw(NVGcontext *ctx) {
    Widget::draw(ctx);
    nvgBeginPath(ctx);
    nvgRect(ctx, m_pos.x(), m_pos.y(), m_size.x(), m_size.y());
    nvgFillColor(ctx, m_selected ? Color(255, 255, 255, 100) : Color(100, 100, 100, 50));
    nvgFill(ctx);
    nvgStrokeColor(ctx, Color(0, 0, 0, 255));
    nvgStroke(ctx);
    nvgFillColor(ctx, Color(255, 255, 255, 255)); // Change text color to white
    nvgText(ctx, m_pos.x() + 10, m_pos.y() + m_size.y() / 2, m_caption.c_str(), nullptr);
    if (m_expanded) {
        for (auto child : m_children) {
            child->draw(ctx);
        }
    }
}

Vector2i TreeViewItem::preferred_size(NVGcontext *ctx) {
    return Vector2i(100, 25);
}

bool TreeViewItem::mouse_button_event(const Vector2i &p, int button, bool down, int modifiers) {
    if (button == GLFW_MOUSE_BUTTON_1 && down) {
        m_expanded = !m_expanded;
        
		m_tree->set_selected(std::dynamic_pointer_cast<TreeViewItem>(shared_from_this()));
        
        return true;
    }
    return false;
}

std::shared_ptr<TreeViewItem> TreeViewItem::add_node(const std::string &caption, std::function<void()> callback) {
	auto child = std::make_shared<TreeViewItem>(shared_from_this(), m_tree, caption, callback);
    m_children.push_back(child);
    return child;
}

NAMESPACE_END(nanogui)
