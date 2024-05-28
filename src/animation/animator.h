#ifndef ANIM_ANIMATION_ANIMATOR_H
#define ANIM_ANIMATION_ANIMATOR_H

#include <glm/glm.hpp>
#include <vector>
#include <memory>

class Scene;

namespace ui{
struct UiContext;
}

namespace anim
{

class SharedResources;

const unsigned int MAX_BONE_NUM = 128;


class Model;
class Shader;
class Animation;
class StackedAnimation;
class Entity;
class AnimationComponent;
class ISerializableSequence;
class RuntimeCinematicSequence;


enum class AnimatorMode {
	Sequence,
	Animation,
	Map
};

class Animator
{
public:
	Animator(std::shared_ptr<SharedResources> resources);
	void update(float dt);
	void update_sequencer(Scene& scene, ui::UiContext& ui_context);
	void update_animation(std::shared_ptr<AnimationComponent> animation, std::shared_ptr<Entity> root, Shader *shader);
	void update_standard_animation(std::shared_ptr<Entity> root, Shader *shader);
	void apply_bone_offsets(std::shared_ptr<Entity> entity, const glm::mat4 &parentTransform);
	void calculate_bone_transform(std::shared_ptr<Entity> entity, std::shared_ptr<AnimationComponent> animationComponent, std::shared_ptr<StackedAnimation> currentAnimation, const glm::mat4 &parentTransform);
	void calculate_root_transform(std::shared_ptr<Entity> entity, std::shared_ptr<AnimationComponent> animationComponent, const glm::mat4 &parentTransform);
	void calculate_standard_bone_transform(std::shared_ptr<Entity> entity, const glm::mat4 &parentTransform);
	void calculate_mesh_transform(std::shared_ptr<Entity> entity, std::shared_ptr<AnimationComponent> animationComponent, std::vector<std::shared_ptr<StackedAnimation>>& animationStack, const glm::mat4 &parentTransform);
	const float get_current_frame_time() const;
	const float get_start_time() const;
	const float get_end_time() const;
	const float get_fps() const;
	const float get_direction() const;
	const bool get_is_stop() const;
	void set_current_time(float current_time);
	void set_start_time(float time);
	void set_end_time(float time);
	void set_fps(float fps);
	void set_direction(bool is_left);
	void set_is_stop(bool is_stop);
	void set_sequencer_end_time(float time);
	void set_mode(AnimatorMode mode);
	void set_is_export(bool is_export){
		is_export_ = is_export;
	}
	
	float get_sequencer_end_time(){
		return sequencer_end_time_;
	}


	void set_active_cinematic_sequence(std::shared_ptr<ISerializableSequence> cinematic_sequence){
		active_cinematic_sequence_ = cinematic_sequence;
	}
	
	std::shared_ptr<ISerializableSequence> get_active_cinematic_sequence(){
		return active_cinematic_sequence_;
	}
	
	void set_active_animation_sequence(std::shared_ptr<ISerializableSequence> cinematic_sequence){
		active_animation_sequence_ = cinematic_sequence;
	}

	std::shared_ptr<ISerializableSequence> get_active_animation_sequence(){
		return active_animation_sequence_;
	}

	void set_runtime_cinematic_sequence_stack(std::vector<std::shared_ptr<RuntimeCinematicSequence>> stack){
		runtime_cinematic_sequences_ = stack;
	}
	
	bool mIsRootMotion{false};
	
private:
	bool is_stop_{true};
	bool is_export_{false};
	float current_time_{0.0f};
	float fps_{60.0f};
	float start_time_{0.0f};
	float end_time_{300.0f};
	float sequencer_end_time_{300.0f};
	float direction_{1.0f};
	std::vector<glm::mat4> final_bone_matrices_;

	AnimatorMode mode_ = AnimatorMode::Sequence;
	
	
	std::shared_ptr<SharedResources> _resources;
	std::shared_ptr<ISerializableSequence> active_cinematic_sequence_;
	std::shared_ptr<ISerializableSequence> active_animation_sequence_;
	std::vector<std::shared_ptr<RuntimeCinematicSequence>> runtime_cinematic_sequences_;
};
}
#endif

