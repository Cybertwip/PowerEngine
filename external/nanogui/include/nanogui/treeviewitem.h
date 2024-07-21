#pragma once

#include <nanogui/nanogui.h>
#include <vector>

NAMESPACE_BEGIN(nanogui)

class TreeViewItem : public Widget {
public:
    TreeViewItem(Widget *parent, const std::string &caption);

    virtual void draw(NVGcontext *ctx) override;
    virtual Vector2i preferred_size(NVGcontext *ctx) const override;
    virtual bool mouse_button_event(const Vector2i &p, int button, bool down, int modifiers) override;

    TreeViewItem* add_node(const std::string &caption);

    void set_expanded(bool expanded) { m_expanded = expanded; }
    bool expanded() const { return m_expanded; }

private:
    std::string m_caption;
    bool m_expanded;
    std::vector<TreeViewItem *> m_children;
};


NAMESPACE_END(nanogui)

