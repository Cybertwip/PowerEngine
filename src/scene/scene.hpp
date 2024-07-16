#ifndef SRC_SCENE_SCENE_H
#define SRC_SCENE_SCENE_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>
#include <memory>
#include <entity/components/component.h>
#include <../resources/shared_resources.h>
#include <../entity/entity.h>
#include <../entity/components/pose_component.h>
#include <../entity/components/renderable/armature_component.h>

#include <glcpp/camera.h>

namespace glcpp
{
    class Camera;
}
namespace anim
{
    class Framebuffer;
}

namespace physics{
class PhysicsWorld;
}

namespace ui {
	struct UiContext;
}

class Scene
{
public:
    Scene() = default;
    virtual ~Scene() = default;
    virtual void init_framebuffer(uint32_t width, uint32_t height) = 0;
    virtual void pre_draw(ui::UiContext& ui_context) = 0;
    virtual void draw(ui::UiContext& ui_context) = 0;
    virtual void picking(int x, int y, bool is_edit, bool is_only_bone) = 0;
	virtual std::shared_ptr<anim::Entity> get_mutable_selected_entity()
	{
		return selected_entity_;
	}
	virtual std::shared_ptr<anim::Framebuffer> get_mutable_framebuffer()
	{
		return framebuffer_;
	}
	virtual std::shared_ptr<anim::SharedResources> get_mutable_shared_resources()
	{
		return resources_;
	}
	virtual std::shared_ptr<anim::Entity> &get_active_camera()
	{
		return camera_;
	}
    virtual void set_size(uint32_t width, uint32_t height)
    {
        if (width > 0)
            width_ = width;
        if (height > 0)
            height_ = height;
    }
    virtual void set_delta_time(float dt)
    {
        delta_time_ = dt;
        resources_->set_dt(delta_time_);
    }
    void set_selected_entity(std::shared_ptr<anim::Entity> entity)
    {
        if (selected_entity_)
        {
            selected_entity_->set_is_selected(false);
        }
        if (entity)
        {
            entity->set_is_selected(true);
        }

        selected_entity_ = entity;
    }
    void set_selected_entity(int id)
    {
        if (id == -1 && selected_entity_)
        {
            id = selected_entity_->get_mutable_root()->get_id();
        }
        set_selected_entity(resources_->get_entity(id));
    }
	
	
	void set_selected_camera(std::shared_ptr<anim::Entity> camera){
		if(find_camera(camera)){
			camera_ = camera;
		}
	}
	
	const float get_width() const {
		return width_;
	}

	const float get_height() const {
		return height_;
	}
	
	
	glm::vec4& get_background_color(){
		return background_color_;
	}
	
	std::vector<std::shared_ptr<anim::Entity>>& get_cameras() {
		return cameras_;
	}

	void add_camera(std::shared_ptr<anim::Entity> camera){
		auto it = std::find(cameras_.begin(), cameras_.end(), camera);
		
		if(it == cameras_.end()){
			if(cameras_.empty()){
				camera_ = camera;
			}
			cameras_.push_back(camera);
		}
	}
	
	std::shared_ptr<anim::Entity> get_camera(int cameraId){
		auto it = std::find_if(cameras_.begin(), cameras_.end(), [cameraId](auto camera){
			return camera->get_id() == cameraId;
		});
		
		if(it != cameras_.end()){
			return *it;
		}
		
		return nullptr;
	}
	
	bool find_camera(std::shared_ptr<anim::Entity> camera){
		auto it = std::find(cameras_.begin(), cameras_.end(), camera);
		
		if(it != cameras_.end()){
			return true;
		}
		
		return false;
	}
	
	bool remove_camera(std::shared_ptr<anim::Entity> camera){
		if(cameras_.size() > 1){
			auto it = std::find(cameras_.begin(), cameras_.end(), camera);
			
			if(it != cameras_.end()){
				cameras_.erase(it);
				
				camera_ = cameras_.back();
				
				return true;
			}
		}
		
		return false;
	}
	
	void set_current_camera(std::shared_ptr<anim::Entity> camera){
		auto it = std::find(cameras_.begin(), cameras_.end(), camera);
		
		if(it != cameras_.end()){
			camera_ = *it;
		}
	}
	
	void clear_cameras(){
		cameras_.clear();
	}

	
	void add_light(std::shared_ptr<anim::Entity> light){
		auto it = std::find(lights_.begin(), lights_.end(), light);
		
		if(it == lights_.end()){
			lights_.push_back(light);
		}
	}
	
	std::vector<std::shared_ptr<anim::Entity>>& get_lights() {
		return lights_;
	}

	
	std::shared_ptr<anim::Entity> get_light(int lightId){
		auto it = std::find_if(lights_.begin(), lights_.end(), [lightId](auto light){
			return light->get_id() == lightId;
		});
		
		if(it != lights_.end()){
			return *it;
		}
		
		return nullptr;
	}
	
	bool find_light(std::shared_ptr<anim::Entity> light){
		auto it = std::find(lights_.begin(), lights_.end(), light);
		
		if(it != lights_.end()){
			return true;
		}
		
		return false;
	}
	
	void remove_light(std::shared_ptr<anim::Entity> light){
		auto it = std::find(lights_.begin(), lights_.end(), light);
		
		if(it != lights_.end()){
			lights_.erase(it);
		}
	}
	void clear_lights(){
		lights_.clear();
	}
	
	physics::PhysicsWorld& get_physics_world(){
		return *_physics_world;
	}
	
protected:
	std::shared_ptr<anim::Entity> selected_entity_{nullptr};
	std::shared_ptr<anim::SharedResources> resources_;
	std::shared_ptr<anim::Framebuffer> framebuffer_;
	std::shared_ptr<anim::Framebuffer> entity_framebuffer_;
	std::vector<std::shared_ptr<anim::Entity>> cameras_;
	std::vector<std::shared_ptr<anim::Entity>> lights_;
	std::shared_ptr<anim::Entity> camera_;
	std::shared_ptr<physics::PhysicsWorld> _physics_world;

    float delta_time_ = 0.0f;
    uint32_t width_ = 800;
    uint32_t height_ = 600;
	
	glm::vec4 background_color_{0.3f, 0.3f, 0.3f, 1.0f};

};

#endif
