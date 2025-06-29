#pragma once

#include <glm/glm.hpp>

#include <nanogui/vector.h>

class Actor;
class TransformComponent;

class CameraComponent
{
public:
	enum class ECameraTag {
		Engine,
		Game
	};
	
	CameraComponent(TransformComponent& transformComponent, float fov, float near, float far, float aspect);
    void update_view();
    
    const nanogui::Matrix4f& get_view() const {
        return mView;
    }
    
    const nanogui::Matrix4f& get_projection() const {
        return mProjection;
    }
	
	void look_at(Actor& actor);
	void look_at(const glm::vec3& position);
	void set_aspect_ratio(float ratio);
	
	void set_tag(ECameraTag tag) {
		mTag = tag;
	}
	
private:
	TransformComponent& mTransformComponent;
	
    float mFov;
    float mNear;
    float mFar;
    float mAspect;
    nanogui::Matrix4f mView;
    nanogui::Matrix4f mProjection;
	
	ECameraTag mTag;
};
