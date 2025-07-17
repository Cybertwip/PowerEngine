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
	CameraComponent(TransformComponent& transformComponent, float fov = 45.0f, float near = 0.01f, float far = 1e5f, float aspect = 16.0f / 9.0f);
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
	
	float get_fov() const {
		return mFov;
	}

	float get_near() const {
		return mNear;
	}
	
	float get_far() const {
		return mFar;
	}
	
	float get_aspect() const {
		return mAspect;
	}
	
	void set_fov(float fov) {
		mFov = fov;
		update_view();
	}
	
	void set_near(float near) {
		mNear = near;
		update_view();
	}

	void set_far(float far) {
		mFar = far;
		update_view();
	}

	void set_aspect(float aspect) {
		mAspect = aspect;
		update_view();
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
