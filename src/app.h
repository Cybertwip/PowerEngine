#ifndef SRC_PIXEL3D_H
#define SRC_PIXEL3D_H

#define IMGUI_DEFINE_MATH_OPERATORS
#include <application.h>

#include "UI/main_layer.h"
#include "event/event_history.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <memory>
//#include "glcpp/application.hpp"

struct GLFWwindow;
class Scene;

class App : public Application
{
public:
	using Application::Application;

	void OnStart() override;
	
	void OnStop() override;
	
	void OnFrame(float dt) override;
	
    void update(float dt);

    void draw_scene(float dt);
    void post_draw();
    void process_input(float dt);

    static void framebuffer_size_callback(GLFWwindow *window, int width, int height);
    // mouse event
    bool is_pressed_ = false;
    bool is_pressed_scroll_ = false;
    glm::vec2 prev_mouse_{-1.0f, -1.0f}, cur_mouse_{-1.0f, -1.0f};
    uint32_t current_scene_idx_ = 0u;
    std::unique_ptr<ui::MainLayer> ui_;
    // timing
    bool is_manipulated_ = false;
    bool is_dialog_open_ = false;
    std::vector<std::shared_ptr<Scene>> scenes_;

private:
//    void init_window(uint32_t width, uint32_t height, const std::string &title);
    void init_callback();
    void init_ui(int width, int height);
    void init_shared_resources();
    void init_scene(uint32_t width, uint32_t height);
    void post_update();
    void process_menu_context();
	void process_ai_context();
    void process_timeline_context();
    void process_scene_context();
    void process_component_context();
    void process_python_context();

	void import_model(const char *const path);
	void import_animation(const char *const path);
    std::shared_ptr<anim::SharedResources> shared_resources_;
    std::unique_ptr<anim::EventHistoryQueue> history_;
};

#endif
