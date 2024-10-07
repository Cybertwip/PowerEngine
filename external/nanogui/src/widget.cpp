/*
 Optimized src/widget.cpp -- Base class of all widgets
 
 NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
 The widget drawing code is based on the NanoVG demo application
 by Mikko Mononen.
 
 All rights reserved. Use of this source code is governed by a
 BSD-style license that can be found in the LICENSE.txt file.
 */

#include <nanogui/widget.h>
#include <nanogui/layout.h>
#include <nanogui/theme.h>
#include <nanogui/window.h>
#include <nanogui/opengl.h>
#include <nanogui/screen.h>

/* Uncomment the following definition to draw red bounding
 boxes around widgets (useful for debugging drawing code) */

// #define NANOGUI_SHOW_WIDGET_BOUNDS 1

NAMESPACE_BEGIN(nanogui)

Widget::Widget(std::weak_ptr<Widget> parent)
: m_parent(parent), m_theme(nullptr), m_layout(nullptr),
m_pos(0), m_size(0), m_fixed_size(0), m_visible(true), m_enabled(true),
m_focused(false), m_mouse_focus(false), m_tooltip(""), m_font_size(-1.f),
m_icon_extra_scale(1.f), m_cursor(Cursor::Arrow), m_screen(std::weak_ptr<Screen>()), m_initialized(false) {
}

void Widget::initialize() {
	assert(!m_initialized);
	
	if (m_parent.lock())
		m_parent.lock()->add_child(shared_from_this());
	
	for (auto child : m_children) {
		child->initialize();
	}
	
	m_initialized = true;
}

Widget::~Widget() {
	if (screen()) {
		screen()->remove_from_focus(shared_from_this());
	}

	if (std::uncaught_exceptions() > 0) {
		/* If a widget constructor throws an exception, it is immediately
		 deallocated but may still be referenced by a parent. Be conservative
		 and don't decrease the reference count of children while dispatching
		 exceptions. */
		return;
	}
}

void Widget::set_theme(std::shared_ptr<Theme> theme) {
	if (m_theme == theme)
		return;
	m_theme = theme;
	for (auto child : m_children)
		child->set_theme(theme);
}

int Widget::font_size() const {
	return (m_font_size < 0 && m_theme) ? m_theme->m_standard_font_size : m_font_size;
}

Vector2i Widget::preferred_size(NVGcontext *ctx) {
	if (m_layout)
		return m_layout->preferred_size(ctx, shared_from_this());
	else
		return m_size;
}

void Widget::perform_layout(NVGcontext *ctx) {	
	if (m_layout) {
		m_layout->perform_layout(ctx, shared_from_this());
	} else {
		for (auto c : m_children) {
			const Vector2i &pref = c->preferred_size(ctx);
			const Vector2i &fix = c->fixed_size();
			c->set_size(Vector2i(
								 fix[0] ? fix[0] : pref[0],
								 fix[1] ? fix[1] : pref[1]
								 ));
			c->perform_layout(ctx);
		}
	}
}

std::shared_ptr<Widget> Widget::find_widget(const Vector2i &p, bool absolute) {
	auto pos = m_pos;
	
	if (absolute) {
		pos = absolute_position();
	}
	for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
		std::shared_ptr<Widget> child = *it;
		if (child->visible() && child->contains(p - pos, absolute))
			return child->find_widget(p - pos);
	}
	return contains(p, absolute) ? shared_from_this() : nullptr;
}

bool Widget::mouse_button_event(const Vector2i &p, int button, bool down, int modifiers) {
	for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
		std::shared_ptr<Widget> child = *it;
		if (child->visible() && child->contains(p - m_pos) &&
			child->mouse_button_event(p - m_pos, button, down, modifiers))
			return true;
	}
	if (button == GLFW_MOUSE_BUTTON_1 && down && !m_focused)
		request_focus();
	return false;
}

bool Widget::mouse_motion_event(const Vector2i &p, const Vector2i &rel, int button, int modifiers) {
	bool handled = false;
	
	for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
		std::shared_ptr<Widget> child = *it;
		if (!child->visible())
			continue;
		
		bool contained      = child->contains(p - m_pos),
		prev_contained = child->contains(p - m_pos - rel);
		
		if (contained != prev_contained)
			handled |= child->mouse_enter_event(p, contained);
		
		if (contained || prev_contained)
			handled |= child->mouse_motion_event(p - m_pos, rel, button, modifiers);
	}
	
	return handled;
}

bool Widget::scroll_event(const Vector2i &p, const Vector2f &rel) {
	for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
		std::shared_ptr<Widget> child = *it;
		if (!child->visible())
			continue;
		if (child->contains(p - m_pos) && child->scroll_event(p - m_pos, rel))
			return true;
	}
	return false;
}

bool Widget::mouse_drag_event(const Vector2i &, const Vector2i &, int, int) {
	return false;
}

bool Widget::mouse_enter_event(const Vector2i &, bool enter) {
	m_mouse_focus = enter;
	return false;
}

bool Widget::focus_event(bool focused) {
	m_focused = focused;
	return false;
}

bool Widget::keyboard_event(int, int, int, int) {
	return false;
}

bool Widget::keyboard_character_event(unsigned int) {
	return false;
}

void Widget::add_child(int index, std::shared_ptr<Widget> widget) {
	assert(index <= child_count());
	m_children.insert(m_children.begin() + index, widget);
	widget->set_parent(shared_from_this());
	widget->set_theme(m_theme);
	widget->set_screen(m_screen); // Update screen pointer
}

void Widget::add_child(std::shared_ptr<Widget> widget) {
	add_child(child_count(), widget);
}

void Widget::remove_child(std::shared_ptr<Widget> widget) {
	size_t child_count = m_children.size();
	m_children.erase(std::remove(m_children.begin(), m_children.end(), widget),
					 m_children.end());
	if (m_children.size() == child_count)
		throw std::runtime_error("Widget::remove_child(): widget not found!");
	if (screen())
		screen()->remove_from_focus(widget);
	widget->set_parent(std::weak_ptr<Widget>());
}

void Widget::remove_child_at(int index) {
	if (index < 0 || index >= (int) m_children.size())
		throw std::runtime_error("Widget::remove_child_at(): out of bounds!");
	std::shared_ptr<Widget> widget = m_children[index];
	m_children.erase(m_children.begin() + index);
	
	if (screen())
		screen()->remove_from_focus(widget);
	widget->set_parent(std::weak_ptr<Widget>());
}
void Widget::shed_children() {
	// Continue removing children until m_children is empty
	while (!m_children.empty()) {
		// Get the last child
		std::shared_ptr<Widget> child = m_children.back();
		
		// Remove the child from m_children
		m_children.pop_back();
		
		// Recursively shed the child's children
		child->shed_children();
		
		// If the widget is part of a screen, remove it from focus
		if (screen()) {
			screen()->remove_from_focus(child);
		}
		
		// Detach the child from its parent and decrement its reference count
		child->set_parent(std::weak_ptr<Widget>());
	}
}

int Widget::child_index(std::shared_ptr<Widget> widget) const {
	auto it = std::find(m_children.begin(), m_children.end(), widget);
	if (it == m_children.end())
		return -1;
	return (int)(it - m_children.begin());
}

std::shared_ptr<Window> Widget::window() {
	std::shared_ptr<Widget> widget = shared_from_this();
	while (widget) {
		auto win = std::dynamic_pointer_cast<Window>(widget);
		if (win)
			return win;
		widget = widget->parent();
	}
	return nullptr;
}

std::shared_ptr<Screen> Widget::screen() {
	return m_screen.lock(); // Directly return the cached screen pointer
}

void Widget::request_focus() {
	if (screen())
		screen()->update_focus(shared_from_this());
}

void Widget::draw(NVGcontext *ctx) {
#if defined(NANOGUI_SHOW_WIDGET_BOUNDS)
	nvgStrokeWidth(ctx, 1.0f);
	nvgBeginPath(ctx);
	nvgRect(ctx, m_pos.x() - 0.5f, m_pos.y() - 0.5f,
			m_size.x() + 1, m_size.y() + 1);
	nvgStrokeColor(ctx, nvgRGBA(255, 0, 0, 255));
	nvgStroke(ctx);
#endif
	
	if (m_children.empty())
		return;
	
	nvgTranslate(ctx, m_pos.x(), m_pos.y());
	for (auto child : m_children) {
		if (!child->visible())
			continue;
#if !defined(NANOGUI_SHOW_WIDGET_BOUNDS)
		nvgSave(ctx);
		nvgIntersectScissor(ctx, child->m_pos.x(), child->m_pos.y(),
							child->m_size.x(), child->m_size.y());
#endif
		
		child->draw(ctx);
		
#if !defined(NANOGUI_SHOW_WIDGET_BOUNDS)
		nvgRestore(ctx);
#endif
	}
	nvgTranslate(ctx, -m_pos.x(), -m_pos.y());
}

void Widget::set_parent(std::weak_ptr<Widget> parent) {
	m_parent = parent;
	m_screen = this->parent() ? this->parent()->screen() : nullptr; // Update screen pointer
	
	if (screen())
		screen()->remove_from_focus(shared_from_this());
}

void Widget::set_screen(std::weak_ptr<Screen> screen) {
	m_screen = screen;
	// Propagate to children
	for (auto child : m_children) {
		child->set_screen(screen);
	}
}

NAMESPACE_END(nanogui)
