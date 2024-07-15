#ifndef UI_IMGUI_MENU_BAR_LAYER_H
#define UI_IMGUI_MENU_BAR_LAYER_H

#include "resources_layer.h"
#include "timeline_layer.h"
#include "hierarchy_layer.h"
#include "component_layer.h"

#include "httplib.h"

#include <string>
#include <memory>
#include <map>
#include <vector>
#include <chrono>
#include <mutex>

struct GLFWwindow;

class Scene;
namespace anim
{
    class SharedResources;
    class Entity;
}

namespace deepmotion {
struct Model {
	std::string id;
	std::string name;
	std::string url;
	std::string timestamp;
};
}

namespace ui
{
    class SceneLayer;

    /**
     * @brief dock + menu bar(import, export)
     *
     */
    class MainLayer
    {
    public:
		MainLayer(Scene& scene, ImTextureID gearTexture, ImTextureID poweredbyTexture, ImTextureID dragAndDropTexture, int width, int height);
        ~MainLayer();

        void init();
        void begin();
        void end();
        void draw_ai_widget(Scene* scene);
		void draw_ingame_menu(Scene* scene);
        void draw_scene(const std::string &title, Scene *scene);
        void draw_component_layer(Scene *scene);
		void serialize_timeline();
		void disable_entity_from_sequencer(std::shared_ptr<anim::Entity> entity);
		void disable_camera_from_sequencer(std::shared_ptr<anim::Entity> entity);
		void disable_light_from_sequencer(std::shared_ptr<anim::Entity> entity);
		void remove_entity_from_sequencer(std::shared_ptr<anim::Entity> entity);
		void remove_camera_from_sequencer(std::shared_ptr<anim::Entity> entity);
		void remove_light_from_sequencer(std::shared_ptr<anim::Entity> entity);
        void draw_hierarchy_layer(Scene *scene);
		void init_timeline(Scene *scene);
		void draw_timeline(Scene *scene);
		void draw_resources(Scene *scene);
		bool is_scene_layer_hovered(const std::string &title);
		bool is_scene_layer_focused(const std::string &title);

        const UiContext &get_context() const;

    private:
        void init_bookmark();
        void shutdown();
        void draw_menu_bar(float fps);
		
		void PollJobStatus();
//        void draw_python_modal(bool &is_open);
        // https://www.fluentcpp.com/2017/09/22/make-pimpl-using-unique_ptr/
        std::map<std::string, std::unique_ptr<SceneLayer>> scene_layer_map_;
        HierarchyLayer hierarchy_layer_{};
        ComponentLayer component_layer_{};
		TimelineLayer timeline_layer_;
		ResourcesLayer resources_layer_{timeline_layer_};
        UiContext context_{};
		
		ImTextureID gearTextureId;
		ImTextureID poweredByTextureId;
		ImTextureID dragAndDropTextureId;

		ImVec2 gearPosition;
		bool draggingThisFrame = false;
		ImVec2 dragStartPosition = {0.0f, 0.0f}; // Track start position of dragging
		ImVec2 gearSize = { 100, 100 };
		
		float animationProgress = 0.0f; // Progress of the animation, from 0.0 to 1.0
		float animationSpeed = 0.1f; // Speed of the animation

		int _windowWidth;
		int _windowHeight;
		
		httplib::SSLClient _client = httplib::SSLClient("api-saymotion.deepmotion.com", 443);
		
		std::string _sessionCookie;
		
		std::vector<deepmotion::Model> _deepmotionModels;
		
		std::shared_ptr<anim::Entity> _ai_entity;
		
		std::vector<std::string> _requestQueue;
		std::mutex _requestQueueMutex;
		bool pollingActive = false;
		std::chrono::time_point<std::chrono::steady_clock> lastPollTime;

    };
}

#endif
