/*
 nanogui/canvas.cpp -- Canvas widget for rendering full-fledged
 OpenGL content within its designated area. Very useful for
 displaying and manipulating 3D objects or scenes. Subclass it and
 overload `draw_contents` for rendering.
 
 NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
 The widget drawing code is based on the NanoVG demo application
 by Mikko Mononen.
 
 All rights reserved. Use of this source code is governed by a
 BSD-style license that can be found in the LICENSE.txt file.
 */

#include <nanogui/screen.h>
#include <nanogui/canvas.h>
#include <nanogui/texture.h>
#include <nanogui/renderpass.h>
#include <nanogui/theme.h>
#include <nanogui/opengl.h>
#include "opengl_check.h"

NAMESPACE_BEGIN(nanogui)

Canvas::Canvas(Widget& parent, Screen& screen, uint8_t samples,
			   bool has_depth_buffer, bool has_stencil_buffer)
: Widget(parent, screen), m_draw_border(true), m_samples(samples), m_has_depth_buffer(has_depth_buffer), m_has_stencil_buffer(has_stencil_buffer) {
	m_size = Vector2i(192, 128);
	
#if defined(NANOGUI_USE_GLES)
	m_samples = 1;
#endif

	m_border_color = theme().m_border_light;
			
	std::optional<std::reference_wrapper<Object>> color_texture = std::nullopt,
	depth_texture = std::nullopt;
	
	if (m_has_stencil_buffer && !m_has_depth_buffer)
		throw std::runtime_error("Canvas::Canvas(): has_stencil implies has_depth!");
	
	color_texture = screen;
	
	if (m_has_depth_buffer) {
		depth_texture = screen;
	}
	
	m_attachment_texture = std::shared_ptr<Texture>( new Texture(
																 Texture::PixelFormat::R,
																 Texture::ComponentFormat::Int32,
																 m_size,
																 Texture::InterpolationMode::Bilinear,
																 Texture::InterpolationMode::Bilinear,
																 Texture::WrapMode::Repeat,
																 m_samples, 					 Texture::TextureFlags::RenderTarget
																 ));
	
	m_render_pass = std::shared_ptr<RenderPass>(new RenderPass(
															   { color_texture->get(), *m_attachment_texture},
															   depth_texture,
															   m_has_stencil_buffer ? depth_texture : std::nullopt,
															   std::nullopt
															   ));
	
}

void Canvas::set_background_color(const Color &background_color) {
	m_render_pass->set_clear_color(0, background_color);
}

const Color& Canvas::background_color() const {
	return m_render_pass->clear_color(0);
}

void Canvas::draw_contents() { /* No-op. */ }

void Canvas::draw(NVGcontext *ctx) {
	auto& scr = screen();
	
	float pixel_ratio = scr.pixel_ratio();
	
	Widget::draw(ctx);
	
	scr.nvg_flush();
	
	Vector2i fbsize = m_size;
	Vector2i offset = absolute_position();
	if (m_draw_border)
		fbsize -= 2;
	
	if (m_draw_border)
		offset += Vector2i(1, 1);
	
	fbsize = Vector2i(Vector2f(fbsize) * pixel_ratio);
	offset = Vector2i(Vector2f(offset) * pixel_ratio);
	
	m_render_pass->resize(scr.framebuffer_size());

	m_render_pass->set_viewport(offset, fbsize);

	m_render_pass->begin();
	draw_contents();
	m_render_pass->end();
	
	if (m_draw_border) {
		nvgBeginPath(ctx);
		nvgStrokeWidth(ctx, 1.f);
		nvgStrokeColor(ctx, m_border_color);
		nvgRoundedRect(ctx, m_pos.x() + .5f, m_pos.y() + .5f,
					   m_size.x() - 1.f, m_size.y() - 1.f,
					   theme().m_window_corner_radius);
		nvgStroke(ctx);
	}
	
}

NAMESPACE_END(nanogui)
