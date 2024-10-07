/*
 src/popup.cpp -- Simple popup widget which is attached to another given
 window (can be nested)
 
 NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
 The widget drawing code is based on the NanoVG demo application
 by Mikko Mononen.
 
 All rights reserved. Use of this source code is governed by a
 BSD-style license that can be found in the LICENSE.txt file.
 */

#include <nanogui/popup.h>
#include <nanogui/theme.h>
#include <nanogui/opengl.h>

NAMESPACE_BEGIN(nanogui)

Popup::Popup(std::shared_ptr<Widget> parent, std::shared_ptr<Window> parent_window)
: Window(parent, ""), m_parent_window(parent_window), m_anchor_pos(Vector2i(0)),
m_anchor_offset(30), m_anchor_size(15), m_side(Side::Right) { }

void Popup::perform_layout(NVGcontext *ctx) {
	Widget::perform_layout(ctx);
	
	if (m_side == Side::Left)
		m_anchor_pos.x() -= size().x();
	
	int max_width = 0;
	
	m_size.y() = 0;
	
	// First pass: Calculate total height and maximum width
	for (auto &child : m_children) {
		m_size.y() += child->height() * 1.125f;
		
		if (child->width() > max_width) {
			max_width = child->width();
		}
	}
	
	m_size.x() = max_width;

	
	int offset = 0.0f;
	
	// Second pass: Adjust positions of child widgets
	for (auto &child : m_children) {
		// Ensure the offset is an integer value
		child->set_width(max_width * 0.95f);
		
		child->set_position(child->position() + Vector2i(m_size.x() * 0.5f - child->width() * 0.5f, offset + m_size.y() * 0.5f - child->height()));

		offset += static_cast<int>(child->height());
	}
	
}

void Popup::refresh_relative_placement() {
	if (!m_parent_window)
		return;
	m_parent_window->refresh_relative_placement();
	m_visible &= m_parent_window->visible_recursive();
	m_pos = m_parent_window->position() + m_anchor_pos - Vector2i(0, m_anchor_offset);
}

void Popup::draw(NVGcontext* ctx) {
	refresh_relative_placement();
	
	if (!m_visible)
		return;
	
	int ds = m_theme->m_window_drop_shadow_size,
	cr = m_theme->m_window_corner_radius;
	
	nvgSave(ctx);
	nvgResetScissor(ctx);
	
	/* Draw a drop shadow */
	NVGpaint shadow_paint = nvgBoxGradient(
										   ctx, m_pos.x(), m_pos.y(), m_size.x(), m_size.y(), cr * 2, ds * 2,
										   m_theme->m_drop_shadow, m_theme->m_transparent);
	
	nvgBeginPath(ctx);
	nvgRect(ctx, m_pos.x() - ds, m_pos.y() - ds, m_size.x() + 2 * ds, m_size.y() + 2 * ds);
	nvgRoundedRect(ctx, m_pos.x(), m_pos.y(), m_size.x(), m_size.y(), cr);
	nvgPathWinding(ctx, NVG_HOLE);
	nvgFillPaint(ctx, shadow_paint);
	nvgFill(ctx);
	
	/* Draw window */
	nvgBeginPath(ctx);
	nvgRoundedRect(ctx, m_pos.x(), m_pos.y(), m_size.x(), m_size.y(), cr);
	
	Vector2i base = m_pos + Vector2i(0, m_anchor_offset);
	int sign = -1;
	if (m_side == Side::Left) {
		base.x() += m_size.x();
		sign = 1;
	}
	
	nvgMoveTo(ctx, base.x() + m_anchor_size * sign, base.y());
	nvgLineTo(ctx, base.x() - 1 * sign, base.y() - m_anchor_size);
	nvgLineTo(ctx, base.x() - 1 * sign, base.y() + m_anchor_size);
	
	nvgFillColor(ctx, m_theme->m_window_popup);
	nvgFill(ctx);
	nvgRestore(ctx);
	
	Widget::draw(ctx);
}

NAMESPACE_END(nanogui)
