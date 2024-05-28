#include "post_processing.h"
#include "opengl/image.h"
#include "opengl/framebuffer.h"
#include "shader.h"
#include <glad/glad.h>

namespace anim
{
PostProcessing::PostProcessing()
{
	intermediate_image1x1_.reset(new anim::Image(1, 1));
	init(1, 1);
}
PostProcessing::~PostProcessing()
{
	intermediate_image1x1_ = nullptr;
}
void PostProcessing::init(uint32_t width, uint32_t height)
{
	mFrameBufferWidth = width;
	mFrameBufferHeight = height;
	
	anim::FramebufferSpec spec(width, height, 1, {anim::FramebufferTextureFormat::RGB}, anim::FramebufferTextureFormat::Depth);
	intermediate_framebuffer_.reset(new anim::Framebuffer(spec));
}
void PostProcessing::execute_outline_with_depth_always(anim::Framebuffer *framebuffer, int stencilVal, int stencilMask, const OutlineInfo &outline_info)
{
	auto width = framebuffer->get_width();
	auto height = framebuffer->get_height();
	
	if (width != intermediate_framebuffer_->get_width() ||
		height != intermediate_framebuffer_->get_height())
	{
		init(width, height);
	}
	
	// outline
	outline_shader_->use();
	
	// only fill selected entity (Silhouette)
	glEnable(GL_STENCIL_TEST);
	glDisable(GL_DEPTH_TEST);
	glStencilFunc(GL_EQUAL, stencilVal, stencilMask);
	glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_KEEP);
	glStencilMask(stencilMask);
	{
		outline_shader_->set_vec3("outlineColor", outline_info.color);
		outline_shader_->set_float("outlineWidth", outline_info.width);
		outline_shader_->set_float("outlineThreshold", outline_info.threshold);
		framebuffer->draw(*outline_shader_);
	}

}

void PostProcessing::set_shaders(anim::Shader *screen, anim::Shader *outline)
{
	screen_shader_ = screen;
	outline_shader_ = outline;
}
}
