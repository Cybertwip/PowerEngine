/*
 src/screen.cpp -- Top-level widget and interface between NanoGUI and GLFW
 
 A significant redesign of this code was contributed by Christian Schueller.
 
 NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
 The widget drawing code is based on the NanoVG demo application
 by Mikko Mononen.
 
 All rights reserved. Use of this source code is governed by a
 BSD-style license that can be found in the LICENSE.txt file.
 */

#include <nanogui/screen.h>
#include <nanogui/theme.h>
#include <nanogui/opengl.h>
#include <nanogui/window.h>
#include <nanogui/popup.h>
#include <nanogui/metal.h>
#include <map>
#include <iostream>
#include <future>

#if defined(EMSCRIPTEN)
#  include <emscripten/emscripten.h>
#  include <emscripten/html5.h>
#endif

#if defined(_WIN32)
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  undef APIENTRY

#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
# endif
#  include <windows.h>

#  define GLFW_EXPOSE_NATIVE_WGL
#  define GLFW_EXPOSE_NATIVE_WIN32
#  include <GLFW/glfw3native.h>
#endif

/* Allow enforcing the GL2 implementation of NanoVG */

#if defined(NANOGUI_USE_OPENGL) || defined(NANOGUI_USE_GLES)
#  if defined(NANOGUI_USE_OPENGL)
#    define NANOVG_GL3_IMPLEMENTATION
#  elif defined(NANOGUI_USE_GLES)
#    define NANOVG_GLES2_IMPLEMENTATION
#  endif
#  include <nanovg_gl.h>
#  include "opengl_check.h"
#elif defined(NANOGUI_USE_METAL)
#  include <nanovg_mtl.h>
#endif

#if defined(__APPLE__)
#  define GLFW_EXPOSE_NATIVE_COCOA 1
#  include <GLFW/glfw3native.h>
#endif

#if !defined(GL_RGBA_FLOAT_MODE)
#  define GL_RGBA_FLOAT_MODE 0x8820
#endif

NAMESPACE_BEGIN(nanogui)

Screen * screen1;
GLFWwindow * window1;

#if defined(NANOGUI_GLAD)
static bool glad_initialized = false;
#endif

/* Calculate pixel ratio for hi-dpi devices. */
static float get_screen_pixel_ratio(GLFWwindow *window) {
#if defined(EMSCRIPTEN)
	return emscripten_get_device_pixel_ratio();
#else
	float xscale, yscale;
	glfwGetWindowContentScale(window, &xscale, &yscale);
	return xscale;
#endif
}

#if defined(EMSCRIPTEN)
static EM_BOOL nanogui_emscripten_resize_callback(int eventType, const EmscriptenUiEvent *, void *) {
	double ratio = emscripten_get_device_pixel_ratio();
	
	int w1, h1;
	emscripten_get_canvas_element_size("#canvas", &w1, &h1);
	
	double w2, h2;
	emscripten_get_element_css_size("#canvas", &w2, &h2);
	
	double w3 = w2 * ratio, h3 = h2 * ratio;
	
	if (w1 != (int) w3 || h1 != (int) h3)
		emscripten_set_canvas_element_size("#canvas", w3, h3);
	
	for (auto it: __nanogui_screens) {
		Screen *screen = it.second;
		screen->resize_event(Vector2i((int) w2, (int) h2));
		screen->redraw();
	}
	
	return true;
}
#endif

Screen::Screen(const std::string &caption,
			   bool fullscreen,
			   bool depth_buffer, bool stencil_buffer,
			   bool float_buffer, unsigned int gl_major, unsigned int gl_minor)
: Widget(std::nullopt), m_glfw_window(nullptr), m_nvg_context(nullptr),
m_cursor(Cursor::Arrow), m_background(0.3f, 0.3f, 0.32f, 1.f), m_caption(caption),
m_shutdown_glfw(false), m_fullscreen(fullscreen), m_depth_buffer(depth_buffer),
m_stencil_buffer(stencil_buffer), m_float_buffer(float_buffer), m_redraw(false), m_focused_widget(std::nullopt) {
	memset(m_cursors, 0, sizeof(GLFWcursor *) * (int) Cursor::CursorCount);
	
	// Framebuffer configuration for performance
	glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_FALSE);
	
	glfwWindowHint(GLFW_REFRESH_RATE, 60);

#if defined(__APPLE__)
	glfwWindowHint(GLFW_COCOA_GRAPHICS_SWITCHING, GLFW_FALSE); // Disable graphics switching

//	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_FALSE);
//	glfwWindowHint(GLFW_SCALE_FRAMEBUFFER, GLFW_FALSE);
//	glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
#endif
	
#if defined(NANOGUI_USE_OPENGL)
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
	// Context creation for performance
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); // Choose appropriate version
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_FALSE);
		
#elif defined(NANOGUI_USE_GLES)
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
	glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, NANOGUI_GLES_VERSION);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#elif defined(NANOGUI_USE_METAL)
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#else
#  error Did not select a graphics API!
#endif
	
	int color_bits = 8, depth_bits = 0, stencil_bits = 0;
	
	if (stencil_buffer && !depth_buffer)
		throw std::runtime_error(
								 "Screen::Screen(): stencil_buffer = True requires depth_buffer = True");
	depth_bits = 24;
	stencil_bits = 8;
	if (m_float_buffer)
		color_bits = 16;

	glfwWindowHint(GLFW_RED_BITS, color_bits);
	glfwWindowHint(GLFW_GREEN_BITS, color_bits);
	glfwWindowHint(GLFW_BLUE_BITS, color_bits);
	glfwWindowHint(GLFW_ALPHA_BITS, color_bits);
	glfwWindowHint(GLFW_STENCIL_BITS, stencil_bits);
	glfwWindowHint(GLFW_DEPTH_BITS, depth_bits);
	
#if (defined(NANOGUI_USE_OPENGL) || defined(NANOGUI_USE_METAL)) && defined(GLFW_FLOATBUFFER)
	glfwWindowHint(GLFW_FLOATBUFFER, m_float_buffer ? GL_TRUE : GL_FALSE);
#else
	m_float_buffer = false;
#endif
	
	for (int i = 0; i < 2; ++i) {
		if (fullscreen) {
			GLFWmonitor *monitor = glfwGetPrimaryMonitor();
			const GLFWvidmode *mode = glfwGetVideoMode(monitor);
			m_glfw_window = glfwCreateWindow(mode->width, mode->height,
											 caption.c_str(), monitor, nullptr);
		} else {
			GLFWmonitor *monitor = glfwGetPrimaryMonitor();
			const GLFWvidmode *mode = glfwGetVideoMode(monitor);
			m_glfw_window = glfwCreateWindow(mode->width, mode->height,
											 caption.c_str(), nullptr, nullptr);
		}
		
		if (m_glfw_window == nullptr && m_float_buffer) {
			m_float_buffer = false;
#if defined(GLFW_FLOATBUFFER)
			glfwWindowHint(GLFW_FLOATBUFFER, GL_FALSE);
#endif
			fprintf(stderr, "Could not allocate floating point framebuffer, retrying without..\n");
		} else {
			break;
		}
	}
	
	if (!m_glfw_window) {
		(void) gl_major; (void) gl_minor;
#if defined(NANOGUI_USE_OPENGL)
		throw std::runtime_error("Could not create an OpenGL " +
								 std::to_string(gl_major) + "." +
								 std::to_string(gl_minor) + " context!");
#elif defined(NANOGUI_USE_GLES)
		throw std::runtime_error("Could not create a GLES 2 context!");
#elif defined(NANOGUI_USE_METAL)
		throw std::runtime_error(
								 "Could not create a GLFW window for rendering using Metal!");
#endif
	}
	
#if defined(NANOGUI_USE_OPENGL) || defined(NANOGUI_USE_GLES)
	glfwMakeContextCurrent(m_glfw_window);
#endif
	
	glfwSetInputMode(m_glfw_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	
#if defined(NANOGUI_GLAD)
	if (!glad_initialized) {
		glad_initialized = true;
		if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
			throw std::runtime_error("Could not initialize GLAD!");
		glGetError(); // pull and ignore unhandled errors like GL_INVALID_ENUM
	}
#endif
	
#if defined(NANOGUI_USE_OPENGL)
	if (m_float_buffer) {
		GLboolean float_mode;
		CHK(glGetBooleanv(GL_RGBA_FLOAT_MODE, &float_mode));
		if (!float_mode) {
			fprintf(stderr, "Could not allocate floating point framebuffer.\n");
			m_float_buffer = false;
		}
	}
#endif
	
	glfwGetFramebufferSize(m_glfw_window, &m_fbsize[0], &m_fbsize[1]);
	
#if defined(NANOGUI_USE_OPENGL) || defined(NANOGUI_USE_GLES)
	CHK(glViewport(0, 0, m_fbsize[0], m_fbsize[1]));
	CHK(glClearColor(m_background[0], m_background[1],
					 m_background[2], m_background[3]));
	CHK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
				GL_STENCIL_BUFFER_BIT));
	
	glfwSwapInterval(0);

	glfwSwapBuffers(m_glfw_window);
#endif
	
#if defined(__APPLE__)
	/* Poll for events once before starting a potentially
	 lengthy loading process. This is needed to be
	 classified as "interactive" by other software such
	 as iTerm2 */
	
	glfwPollEvents();
#endif
	
	initialize(m_glfw_window, true);
	
#if defined(NANOGUI_USE_METAL)
	if (depth_buffer) {
		m_depth_texture = std::make_shared<Texture>(
									m_stencil_buffer ? Texture::PixelFormat::DepthStencil
									: Texture::PixelFormat::DepthStencil,
									Texture::ComponentFormat::Float32,
									m_fbsize,
									Texture::InterpolationMode::Bilinear,
									Texture::InterpolationMode::Bilinear,
									Texture::WrapMode::ClampToEdge,
									1,
									Texture::TextureFlags::RenderTarget
									);
	}

#endif
	
	set_screen(*this);
}

void Screen::initialize(GLFWwindow *window, bool shutdown_glfw) {
	m_glfw_window = window;
	m_shutdown_glfw = shutdown_glfw;
	glfwGetWindowSize(m_glfw_window, &m_size[0], &m_size[1]);
	glfwGetFramebufferSize(m_glfw_window, &m_fbsize[0], &m_fbsize[1]);
	
	m_pixel_ratio = get_screen_pixel_ratio(window);
	
#if defined(EMSCRIPTEN)
	double w, h;
	emscripten_get_element_css_size("#canvas", &w, &h);
	double ratio = emscripten_get_device_pixel_ratio(),
	w2 = w * ratio, h2 = h * ratio;
	
	if (w != m_size[0] || h != m_size[1]) {
		/* The canvas element is configured as width/height: auto, expand to
		 the available space instead of using the specified window resolution */
		nanogui_emscripten_resize_callback(0, nullptr, nullptr);
		emscripten_set_resize_callback(nullptr, nullptr, false,
									   nanogui_emscripten_resize_callback);
	} else if (w != w2 || h != h2) {
		/* Configure for rendering on a high-DPI display */
		emscripten_set_canvas_element_size("#canvas", (int) w2, (int) h2);
		emscripten_set_element_css_size("#canvas", w, h);
	}
	m_fbsize = Vector2i((int) w2, (int) h2);
	m_size = Vector2i((int) w, (int) h);
#elif defined(_WIN32) || defined(__linux__)
	if (m_pixel_ratio != 1 && !m_fullscreen)
		glfwSetWindowSize(window, m_size.x() * m_pixel_ratio,
						  m_size.y() * m_pixel_ratio);
#endif
	
#if defined(NANOGUI_GLAD)
	if (!glad_initialized) {
		glad_initialized = true;
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
			throw std::runtime_error("Could not initialize GLAD!");
		glGetError(); // pull and ignore unhandled errors like GL_INVALID_ENUM
	}
#endif
	
	int flags = NVG_ANTIALIAS;
	if (m_stencil_buffer)
		flags |= NVG_STENCIL_STROKES;
#if !defined(NDEBUG)
	flags |= NVG_DEBUG;
#endif
	
#if defined(NANOGUI_USE_OPENGL)
	m_nvg_context = nvgCreateGL3(flags);
#elif defined(NANOGUI_USE_GLES)
	m_nvg_context = nvgCreateGLES2(flags);
#elif defined(NANOGUI_USE_METAL)
	void *nswin;
	m_nswin = nswin = glfwGetCocoaWindow(window);
	metal_window_init(nswin, m_float_buffer);
	metal_window_set_size(nswin, m_fbsize);
	m_nvg_context = nvgCreateMTL(metal_layer(),
								 metal_command_queue(),
								 flags | NVG_TRIPLE_BUFFER);
#endif
	
	if (!m_nvg_context)
		throw std::runtime_error("Could not initialize NanoVG!");
	
	m_visible = glfwGetWindowAttrib(window, GLFW_VISIBLE) != 0;
	m_default_theme = std::make_unique<Theme>(m_nvg_context);
	set_theme(*m_default_theme);
	m_mouse_pos = Vector2i(0);
	m_mouse_state = m_modifiers = 0;
	m_drag_active = false;
	m_last_interaction = glfwGetTime();
	m_process_events = true;
	m_redraw = true;
	window1 = m_glfw_window;
	screen1 = this;
	
	for (size_t i = 0; i < (size_t) Cursor::CursorCount; ++i)
		m_cursors[i] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR + (int) i);
}

Screen::~Screen() {
	screen1 = nullptr;
	for (size_t i = 0; i < (size_t) Cursor::CursorCount; ++i) {
		if (m_cursors[i])
			glfwDestroyCursor(m_cursors[i]);
	}
	
	if (m_nvg_context) {
#if defined(NANOGUI_USE_OPENGL)
		nvgDeleteGL3(m_nvg_context);
#elif defined(NANOGUI_USE_GLES)
		nvgDeleteGLES2(m_nvg_context);
#elif defined(NANOGUI_USE_METAL)
		nvgDeleteMTL(m_nvg_context);
#endif
	}
	
	if (m_glfw_window && m_shutdown_glfw)
		glfwDestroyWindow(m_glfw_window);
}

void Screen::set_visible(bool visible) {
	if (m_visible != visible) {
		m_visible = visible;
		
		if (visible)
			glfwShowWindow(m_glfw_window);
		else
			glfwHideWindow(m_glfw_window);
	}
}

void Screen::set_caption(const std::string &caption) {
	if (caption != m_caption) {
		glfwSetWindowTitle(m_glfw_window, caption.c_str());
		m_caption = caption;
	}
}

void Screen::set_size(const Vector2i &size) {
	Widget::set_size(size);
	
#if defined(_WIN32) || defined(__linux__) || defined(EMSCRIPTEN)
	glfwSetWindowSize(m_glfw_window, size.x() * m_pixel_ratio,
					  size.y() * m_pixel_ratio);
#else
	glfwSetWindowSize(m_glfw_window, size.x(), size.y());
#endif
}

void Screen::clear() {
#if defined(NANOGUI_USE_OPENGL) || defined(NANOGUI_USE_GLES)
	CHK(glClearColor(m_background[0], m_background[1], m_background[2], m_background[3]));
	CHK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));
#elif defined(NANOGUI_USE_METAL)
	mnvgClearWithColor(m_nvg_context, m_background);
#endif
}

void Screen::draw_setup() {
#if defined(NANOGUI_USE_OPENGL) || defined(NANOGUI_USE_GLES)
	glfwMakeContextCurrent(m_glfw_window);
#elif defined(NANOGUI_USE_METAL)
	void *nswin = glfwGetCocoaWindow(m_glfw_window);
	metal_window_set_size(nswin, m_fbsize);
	m_metal_drawable = metal_window_next_drawable(nswin);
	m_metal_texture = metal_drawable_texture(m_metal_drawable);
	mnvgSetColorTexture(m_nvg_context, m_metal_texture);
#endif
	
#if !defined(EMSCRIPTEN)
	glfwGetFramebufferSize(m_glfw_window, &m_fbsize[0], &m_fbsize[1]);
	glfwGetWindowSize(m_glfw_window, &m_size[0], &m_size[1]);
#else
	emscripten_get_canvas_element_size("#canvas", &m_size[0], &m_size[1]);
	m_fbsize = m_size;
#endif
	
#if defined(_WIN32) || defined(__linux__) || defined(EMSCRIPTEN)
	m_fbsize = m_size;
	m_size = Vector2i(Vector2f(m_size) / m_pixel_ratio);
#else
	/* Recompute pixel ratio on OSX */
	if (m_size[0])
		m_pixel_ratio = (float) m_fbsize[0] / (float) m_size[0];
#if defined(NANOGUI_USE_METAL)
	metal_window_set_content_scale(nswin, m_pixel_ratio);
#endif
#endif
	
#if defined(NANOGUI_USE_OPENGL) || defined(NANOGUI_USE_GLES)
	CHK(glViewport(0, 0, m_fbsize[0], m_fbsize[1]));
#endif
}

void Screen::draw_teardown() {
#if defined(NANOGUI_USE_OPENGL) || defined(NANOGUI_USE_GLES)
	
	glfwSwapBuffers(m_glfw_window);

#elif defined(NANOGUI_USE_METAL)
	mnvgSetColorTexture(m_nvg_context, nullptr);
	metal_present_and_release_drawable(m_metal_drawable);
	m_metal_texture = nullptr;
	m_metal_drawable = nullptr;
#endif
}

void Screen::draw_all() {
	if (m_redraw) {
		m_redraw = false;
		
#if defined(NANOGUI_USE_METAL)
		void *pool = autorelease_init();
#endif
		draw_setup();
		draw_contents();
		draw_widgets();
		process_events();
		draw_teardown();
		
#if defined(NANOGUI_USE_METAL)
		autorelease_release(pool);
#endif
	}
}

void Screen::draw_contents() {
	clear();
}

void Screen::nvg_flush() {
	NVGparams *params = nvgInternalParams(m_nvg_context);
	params->renderFlush(params->userPtr);
	params->renderViewport(params->userPtr, m_size[0], m_size[1], m_pixel_ratio);
}

void Screen::draw_widgets() {
	nvgBeginFrame(m_nvg_context, m_size[0], m_size[1], m_pixel_ratio);
	
	draw(m_nvg_context);
	
	double elapsed = glfwGetTime() - m_last_interaction;
	
	if (elapsed > 0.5f) {
		/* Draw tooltips */
		Widget* widget = find_widget(m_mouse_pos);
		if (widget && !widget->tooltip().empty()) {
			int tooltip_width = 150;
			
			float bounds[4];
			nvgFontFace(m_nvg_context, "sans");
			nvgFontSize(m_nvg_context, 15.0f);
			nvgTextAlign(m_nvg_context, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
			nvgTextLineHeight(m_nvg_context, 1.1f);
			Vector2i pos = widget->absolute_position() +
			Vector2i(widget->width() / 2, widget->height() + 10);
			
			nvgTextBounds(m_nvg_context, pos.x(), pos.y(),
						  widget->tooltip().c_str(), nullptr, bounds);
			
			int h = (bounds[2] - bounds[0]) / 2;
			if (h > tooltip_width / 2) {
				nvgTextAlign(m_nvg_context, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
				nvgTextBoxBounds(m_nvg_context, pos.x(), pos.y(), tooltip_width,
								 widget->tooltip().c_str(), nullptr, bounds);
				
				h = (bounds[2] - bounds[0]) / 2;
			}
			int shift = 0;
			
			if (pos.x() - h - 8 < 0) {
				/* Keep tooltips on screen */
				shift = pos.x() - h - 8;
				pos.x() -= shift;
				bounds[0] -= shift;
				bounds[2] -= shift;
			}
			
//			nvgGlobalAlpha(m_nvg_context,
//						   std::min(1.0, 2 * (elapsed - 0.5f)) * 0.8);
			
			nvgBeginPath(m_nvg_context);
			nvgFillColor(m_nvg_context, Color(0, 255));
			nvgRoundedRect(m_nvg_context, bounds[0] - 4 - h, bounds[1] - 4,
						   (int) (bounds[2] - bounds[0]) + 8,
						   (int) (bounds[3] - bounds[1]) + 8, 3);
			
			int px = (int) ((bounds[2] + bounds[0]) / 2) - h + shift;
			nvgMoveTo(m_nvg_context, px, bounds[1] - 10);
			nvgLineTo(m_nvg_context, px + 7, bounds[1] + 1);
			nvgLineTo(m_nvg_context, px - 7, bounds[1] + 1);
			nvgFill(m_nvg_context);
			
			nvgFillColor(m_nvg_context, Color(255, 255));
			nvgFontBlur(m_nvg_context, 0.0f);
			nvgTextBox(m_nvg_context, pos.x() - h, pos.y(), tooltip_width,
					   widget->tooltip().c_str(), nullptr);
		}
	}
	
	nvgEndFrame(m_nvg_context);
}

bool Screen::keyboard_event(int key, int scancode, int action, int modifiers) {
	if (m_focused_widget) {
		m_focused_widget->get().keyboard_event(key, scancode, action, modifiers);
			return true;
	}
	
	return false;
}

bool Screen::keyboard_character_event(unsigned int codepoint) {
	if (m_focused_widget) {
		m_focused_widget->get().keyboard_character_event(codepoint);
		
		return true;
	}
	
	return false;
}

bool Screen::resize_event(const Vector2i& size) {
	if (m_resize_callback)
		m_resize_callback(size);
	
	perform_layout(nvg_context());
	
	m_redraw = true;
	draw_all();
	return true;
}

void Screen::redraw() {
	if (!m_redraw) {
		m_redraw = true;
#if !defined(EMSCRIPTEN)
		glfwPostEmptyEvent();
#endif
	}
}

void Screen::cursor_pos_callback_event(double x, double y) {
	Vector2i p((int) x, (int) y);
	
#if defined(_WIN32) || defined(__linux__) || defined(EMSCRIPTEN)
	p = Vector2i(Vector2f(p) / m_pixel_ratio);
#endif
	
	m_last_interaction = glfwGetTime();
	p -= Vector2i(1, 2);
	
	bool ret = false;
	if (!m_drag_active) {
		Widget* widget = find_widget(p);
		if (widget != nullptr && widget->cursor() != m_cursor) {
			m_cursor = widget->cursor();
			glfwSetCursor(m_glfw_window, m_cursors[(int) m_cursor]);
		}
	} else {
		ret = m_drag_widget->mouse_drag_event(
											  p - m_drag_widget->parent()->get().absolute_position(), p - m_mouse_pos,
											  m_mouse_state, m_modifiers);
	}
	
	if (!ret)
		ret = mouse_motion_event(p, p - m_mouse_pos, m_mouse_state, m_modifiers);
	
	m_mouse_pos = p;
	m_redraw |= ret;
}

void Screen::mouse_button_callback_event(int button, int action, int modifiers) {
	m_modifiers = modifiers;
	m_last_interaction = glfwGetTime();
	
#if defined(__APPLE__)
	if (button == GLFW_MOUSE_BUTTON_1 && modifiers == GLFW_MOD_CONTROL)
		button = GLFW_MOUSE_BUTTON_2;
#endif
	if (m_focused_widget) {
		auto window =
		dynamic_cast<Window*>(&m_focused_widget->get());
		if (window && window->modal()) {
			if (!window->contains(m_mouse_pos))
				return;
		}
	}
	
	
	bool btn12 = button == GLFW_MOUSE_BUTTON_1 || button == GLFW_MOUSE_BUTTON_2;
	
	if (!m_drag_active && action == GLFW_PRESS && btn12) {
		m_drag_widget = find_widget(m_mouse_pos);
		if (m_drag_widget == this)
			m_drag_widget = nullptr;
		m_drag_active = m_drag_widget != nullptr;
	} else if (m_drag_active && action == GLFW_RELEASE && btn12) {
		m_drag_active = false;
		m_drag_widget = nullptr;
	}

	if (action == GLFW_PRESS)
		m_mouse_state |= 1 << button;
	else
		m_mouse_state &= ~(1 << button);
	
	auto drop_widget = find_widget(m_mouse_pos);
	if (m_drag_active && action == GLFW_RELEASE && m_drag_widget &&
		drop_widget != m_drag_widget) {
		m_redraw |= m_drag_widget->mouse_button_event(
													  m_mouse_pos - m_drag_widget->absolute_position(), button,
													  false, m_modifiers);
	}
	
	if (drop_widget != nullptr && drop_widget->cursor() != m_cursor) {
		m_cursor = drop_widget->cursor();
		glfwSetCursor(m_glfw_window, m_cursors[(int) m_cursor]);
	}
	
	m_redraw |= mouse_button_event(m_mouse_pos, button,
								   action == GLFW_PRESS, m_modifiers);

}

void Screen::key_callback_event(int key, int scancode, int action, int mods) {
	m_last_interaction = glfwGetTime();
	m_redraw |= keyboard_event(key, scancode, action, mods);
}

void Screen::char_callback_event(unsigned int codepoint) {
	m_last_interaction = glfwGetTime();
	m_redraw |= keyboard_character_event(codepoint);
}

void Screen::drop_callback_event(int count, const char **filenames) {
	std::vector<std::string> arg(count);
	for (int i = 0; i < count; ++i)
		arg[i] = filenames[i];
	m_redraw |= drop_event(*this, arg);
}

void Screen::scroll_callback_event(double x, double y) {
	m_last_interaction = glfwGetTime();
	if (m_focused_widget) {
		auto window =
		dynamic_cast<Window*>(&m_focused_widget->get());
		if (window && window->modal()) {
			if (!window->contains(m_mouse_pos))
				return;
		}
	}
	m_redraw |= scroll_event(m_mouse_pos, Vector2f(x, y));
}

void Screen::resize_callback_event(int, int) {
#if defined(EMSCRIPTEN)
	return;
#endif
	
	Vector2i fb_size, size;
	glfwGetFramebufferSize(m_glfw_window, &fb_size[0], &fb_size[1]);
	glfwGetWindowSize(m_glfw_window, &size[0], &size[1]);
	if (fb_size == Vector2i(0, 0) || size == Vector2i(0, 0))
		return;
	m_fbsize = fb_size; m_size = size;
	
#if defined(_WIN32) || defined(__linux__) || defined(EMSCRIPTEN)
	m_size = Vector2i(Vector2f(m_size) / m_pixel_ratio);
#endif
	
	m_last_interaction = glfwGetTime();
	
#if defined(NANOGUI_USE_METAL)
	if (m_depth_texture)
		m_depth_texture->resize(fb_size);
#endif

	resize_event(m_size);
	redraw();
}
// src/screen.cpp

void Screen::update_focus(Widget& widget) {
	if (&widget == this){
		return;
	}
	
	if (m_focused_widget) {
		m_focused_widget->get().focus_event(false);
	}
	
	m_focused_widget = std::ref(widget);
	
	m_focused_widget->get().focus_event(true);

	if (auto* window = dynamic_cast<Window*>(&widget); window) {
		if (window->modal()) {
			move_window_to_front(*window);
		}
	}
}


void Screen::remove_from_focus(Widget& widget) {
	if (m_focused_widget) {
		if (&widget == &m_focused_widget->get()){
			m_focused_widget = std::nullopt;
		}
	}
}

void Screen::center_window(Window& window) {
	if (window.size() == 0) {
		window.set_size(window.preferred_size(m_nvg_context));
		window.perform_layout(m_nvg_context);
	}
	window.set_position((m_size - window.size()) / 2);
}

void Screen::move_window_to_front(Window& window) {
	m_children.erase(std::remove_if(m_children.begin(), m_children.end(), [&window](std::reference_wrapper<Widget> item){
		return &item.get() == &window;
	}), m_children.end());
	m_children.push_back(window);
	/* Brute force topological sort (no problem for a few windows..) */
	bool changed = false;
	do {
		size_t base_index = 0;
		for (size_t index = 0; index < m_children.size(); ++index)
			if (&m_children[index].get() == &window)
				base_index = index;
		changed = false;
		for (size_t index = 0; index < m_children.size(); ++index) {
			auto pw = dynamic_cast<Popup*>(&m_children[index].get());
			if (pw && pw->parent_window() == &window && index < base_index) {
				move_window_to_front(*pw);
				changed = true;
				break;
			}
		}
	} while (changed);
}

bool Screen::tooltip_fade_in_progress() {
	double elapsed = glfwGetTime() - m_last_interaction;
	if (elapsed < 0.25f || elapsed > 1.25f)
		return false;
	/* Temporarily increase the frame rate to fade in the tooltip */
	Widget* widget = find_widget(m_mouse_pos);
	return widget && !widget->tooltip().empty();
}

Texture::PixelFormat Screen::pixel_format() const {
#if defined(NANOGUI_USE_METAL)
	if (!m_float_buffer)
		return Texture::PixelFormat::BGRA;
#endif
	return Texture::PixelFormat::RGBA;
}

Texture::ComponentFormat Screen::component_format() const {
	if (m_float_buffer)
		return Texture::ComponentFormat::Float16;
	else
		return Texture::ComponentFormat::UInt8;
}

#if defined(NANOGUI_USE_METAL)
void *Screen::metal_layer() const {
	return metal_window_layer(glfwGetCocoaWindow(m_glfw_window));
}
#endif


NAMESPACE_END(nanogui)
