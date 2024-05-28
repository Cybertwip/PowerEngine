#include "animation_component.h"

#include "animation/animation.h"

#include "components/renderable/mesh_component.h"

namespace {
const unsigned int MAX_BONE_NUM = 128;
}

namespace anim
{

void AnimationSequence::serialize(){
	
}

void AnimationSequence::deserialize() {
	auto animation = _entity->get_component<AnimationComponent>();
	
	auto children = _entity->get_children_recursive();
	
	auto &name_bone_map = animation->get_animation()->get_mutable_name_bone_map();

	sequencer_.mFrameMax = _resources->get_animation(_animation_id)->get_duration();
	
	for(auto child : children){
		EntityType entityType = EntityType::Bone;
		
		if(child->get_component<MeshComponent>()){
			entityType = EntityType::Mesh;
		}
		
		sequencer_.mItems.push_back(std::make_shared<AnimationItem>(child->get_id(), child->get_name(), entityType, 0, animation->get_custom_duration()));
		
		auto& bone = name_bone_map[child->get_name()];
		
		auto timeset = bone->get_time_set();
		
		if(!timeset.empty()){
			for(auto& time : timeset){
				
				int frame = static_cast<int>(time);
				
				sequencer_.mItems.back()->mKeyFrames[frame] = {};
				
				auto& keyframe = sequencer_.mItems.back()->mKeyFrames[frame];
				
				keyframe.active = true;
				
				keyframe.transform = bone->get_local_transform(frame, bone->get_factor());

			}
		}

	}
	
	if(!children.empty()){
		sequencer_.mSelectedEntry = 0;
	}
}

StackedAnimation::StackedAnimation(std::shared_ptr<Animation> animation, int startTime, int endTime, int offset)
: _animation(animation), _startTime(startTime), _endTime(endTime), _offset(offset), _duration(animation->get_duration()), _weight(1.0f){
}

AnimationComponent::AnimationComponent(){
	final_bone_matrices_.reserve(MAX_BONE_NUM);
	for (unsigned int i = 0U; i < MAX_BONE_NUM; i++)
		final_bone_matrices_.push_back(glm::mat4(1.0f));
}

void AnimationComponent::init_animation()
{
	fps_ = animation_->get_fps();
}
void AnimationComponent::play()
{
	is_stop_ = false;
}
void AnimationComponent::stop()
{
	is_stop_ = true;
}
void AnimationComponent::reload()
{
	if (animation_ && animation_->get_type() == AnimationType::Json)
	{
		animation_->reload();
		init_animation();
	}
}

void AnimationComponent::set_animation(std::shared_ptr<Animation> animation)
{
	animation_ = animation;
	
	if (!animation)
	{
		return;
	}

	init_animation();
}

void AnimationComponent::set_root_bone_name(const std::string& root_bone_name)
{
	animation_->set_root_bone_name(root_bone_name);
}

void AnimationComponent::set_current_frame_num_to_time(uint32_t frame)
{
	current_time_ = static_cast<float>(frame);
}

void AnimationComponent::set_fps(float fps)
{
	if (fps > 0.0f)
	{
		fps_ = fps;
	}
}

std::shared_ptr<Animation> AnimationComponent::get_animation() const
{
	return animation_;
}

const uint32_t AnimationComponent::get_current_frame_num() const
{
	return static_cast<uint32_t>(roundf(current_time_));
}

const uint32_t AnimationComponent::get_custom_duration() const
{
	return animation_->get_duration();
}
void AnimationComponent::set_current_time(float current_time){
	current_time_ = current_time;
}
float AnimationComponent::get_current_time()
{
	return current_time_;
}
float AnimationComponent::get_fps() const
{
	return fps_;
}
const std::string& AnimationComponent::get_root_bone_name() const{
	return animation_->get_root_bone_name();
}
}

