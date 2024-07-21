#include <nanogui/treeviewitem.h>
#include <nanogui/opengl.h>

NAMESPACE_BEGIN(nanogui)

TreeViewItem::TreeViewItem(Widget *parent, const std::string &caption)
    : Widget(parent), m_caption(caption), m_expanded(false) {}

void TreeViewItem::draw(NVGcontext *ctx) {
    Widget::draw(ctx);
    nvgBeginPath(ctx);
    nvgRect(ctx, m_pos.x(), m_pos.y(), m_size.x(), m_size.y());
    nvgFillColor(ctx, m_expanded ? Color(200, 200, 200, 25) : Color(255, 255, 255, 25));
    nvgFill(ctx);
    nvgStrokeColor(ctx, Color(0, 0, 0, 255));
    nvgStroke(ctx);
    nvgFillColor(ctx, Color(0, 0, 0, 255));
    nvgText(ctx, m_pos.x() + 10, m_pos.y() + m_size.y() / 2, m_caption.c_str(), nullptr);
    if (m_expanded) {
        for (auto child : m_children) {
            child->draw(ctx);
        }
    }
}

Vector2i TreeViewItem::preferred_size(NVGcontext *ctx) const {
    return Vector2i(100, 25);
}

bool TreeViewItem::mouse_button_event(const Vector2i &p, int button, bool down, int modifiers) {
    if (button == GLFW_MOUSE_BUTTON_1 && down) {
        m_expanded = !m_expanded;
        return true;
    }
    return false;
}

TreeViewItem* TreeViewItem::add_node(const std::string &caption) {
    TreeViewItem *child = new TreeViewItem(this, caption);
    m_children.push_back(child);
    return child;
}

NAMESPACE_END(nanogui)
