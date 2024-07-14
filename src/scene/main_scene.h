#ifndef SRC_SCENE_MAIN_SCENE_H
#define SRC_SCENE_MAIN_SCENE_H

#include "scene.hpp"

#include <vector>
namespace anim
{
    class Entity;
    class Image;
}
class MainScene : public Scene
{
public:
    MainScene() = delete;
    MainScene(uint32_t width, uint32_t height);
    ~MainScene() = default;
	void init_shadow_map(LightManager& lightManager);
    void init_framebuffer(uint32_t width, uint32_t height) override;
    void pre_draw(ui::UiContext& ui_context) override;
    void draw() override;
    void picking(int x, int y, bool is_edit, bool is_only_bone) override;

private:
    void init_camera();
    void update_framebuffer();
	void render_to_shadow_map(ui::UiContext& ui_context);
	void draw_to_framebuffer(ui::UiContext& ui_context);

	std::shared_ptr<anim::Image> grid_framebuffer_;

	GLuint shadowMap;
	GLuint shadowMapFBO;
};

#endif
