#ifndef ANIM_GRAPHICS_POST_PROCESSING_H
#define ANIM_GRAPHICS_POST_PROCESSING_H

#include <memory>
#include <glm/glm.hpp>

namespace anim
{
    class Framebuffer;
    class Shader;
    class Image;

    struct OutlineInfo
    {
        glm::vec3 color{1.0f, 0.5f, 0.06f};
        float width = 1.25f;
        float threshold = 0.8f;
    };

    class PostProcessing
    {
    public:
        PostProcessing();
        ~PostProcessing();
		void execute_outline_with_depth_always(anim::Framebuffer *framebuffer, int stencilVal, int stencilMask, const OutlineInfo &outline_info = OutlineInfo());
        void set_shaders(anim::Shader *screen, anim::Shader *outline);
		
    private:
        void init(uint32_t width, uint32_t height);

        anim::Shader *screen_shader_;
        anim::Shader *outline_shader_;
        std::shared_ptr<anim::Image> intermediate_image1x1_;
        std::shared_ptr<anim::Framebuffer> intermediate_framebuffer_;
		
		int mFrameBufferWidth;
		int mFrameBufferHeight;
		
    };
}

#endif
