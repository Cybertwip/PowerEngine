#ifndef UI_IMGUI_TIMELINE_LAYER_H
#define UI_IMGUI_TIMELINE_LAYER_H

#include <vector>
#include <memory>
#include <string>
#include "ui_context.h"

class Scene;

namespace glcpp{
class Camera;
}

namespace anim
{
class Entity;
class SharedResources;
class Animation;
class Animator;
class AnimationComponent;
class Bone;
class ISerializableSequence;
class CinematicSequence;
class AnimationSequence;
}

namespace ui
{
class TextEditLayer;
class TimelineLayer
{
public:
	TimelineLayer();
	~TimelineLayer();
	void init(Scene *scene);
	void draw(Scene *scene, UiContext &context);
	
	void setActiveEntity(UiContext &ui_context, std::shared_ptr<anim::Entity> entity);
	void setCinematicSequence(std::shared_ptr<anim::CinematicSequence> sequence);
	void setAnimationSequence(std::shared_ptr<anim::AnimationSequence> sequence);
	void clearActiveEntity();
	void resetAnimatorTime();

	void disable_camera_from_sequencer(std::shared_ptr<anim::Entity> entity);
	void disable_entity_from_sequencer(std::shared_ptr<anim::Entity> entity);
	void disable_light_from_sequencer(std::shared_ptr<anim::Entity> entity);

	void remove_entity_from_sequencer(std::shared_ptr<anim::Entity> entity);
	void remove_camera_from_sequencer(std::shared_ptr<anim::Entity> entity);
	void remove_light_from_sequencer(std::shared_ptr<anim::Entity> entity);

	void serialize_timeline();
	void deserialize_timeline();

private:
	inline void init_context(UiContext &ui_context, Scene *scene);
	
	bool is_hovered_zoom_slider_{false};
	bool is_opened_mesh_{true};
	bool is_opened_armature_{true};
	bool is_recording_{true};
	uint32_t current_frame_{0u};
	Scene *scene_;
	std::shared_ptr<anim::Entity> hierarchy_entity_;
	std::shared_ptr<anim::Entity> root_entity_;
	std::shared_ptr<anim::Entity> animation_entity_;
	
	std::shared_ptr<anim::SharedResources> resources_;
	anim::Animator *animator_ = nullptr;
	
	std::shared_ptr<anim::ISerializableSequence> cinematic_sequence_;
	std::shared_ptr<anim::ISerializableSequence> animation_sequence_;
	std::shared_ptr<anim::Entity> _blueprint_entity;
};
}

#endif
