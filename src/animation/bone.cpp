#include "bone.h"
#include "../util/utility.h"
#include <string>
#include <util/log.h>
#include <glm/gtx/string_cast.hpp>


#include "components/pose_component.h"
#include "entity.h"

namespace anim
{
Bone::Bone() = default;
Bone::Bone(const std::string &name, sfbx::AnimationCurveNode* positionCurve, sfbx::AnimationCurveNode* rotationCurve, sfbx::AnimationCurveNode* scaleCurve, const glm::mat4 &inverse_binding_pose)
: name_(name),
local_transform_(1.0f)
{
	set_bindpose(glm::inverse(inverse_binding_pose));
		
	std::size_t num_positions = positionCurve ?  positionCurve->getAnimationCurves()[0]->getTimes().size() : 0;

	std::size_t num_rotations = rotationCurve ?  rotationCurve->getAnimationCurves()[0]->getTimes().size() : 0;

	std::size_t num_scales = scaleCurve ?  scaleCurve->getAnimationCurves()[0]->getTimes().size() : 0;

	for (int pos_idx = 0; pos_idx < num_positions; ++pos_idx)
	{
		if(positionCurve){
			positionCurve->applyAnimation(pos_idx / 60.0f);
		}
		
		if(rotationCurve){
			rotationCurve->applyAnimation(pos_idx / 60.0f);
		}
		
		if(scaleCurve){
			scaleCurve->applyAnimation(pos_idx / 60.0f);
		}
		
		auto model = sfbx::as<sfbx::Model>(rotationCurve->getAnimationTarget());
		
		auto [t, r, s] = DecomposeTransform(SfbxMatToGlmMat(model->getLocalMatrix()));
		
		glm::vec3 rotationAxis = glm::degrees(glm::eulerAngles(r));
		
		push_position(t, pos_idx);
		push_rotation(r, pos_idx);
		push_scale(s, pos_idx);
	}
	
	for (int rot_idx = 0; rot_idx < num_rotations; ++rot_idx)
	{
		if(positionCurve){
			positionCurve->applyAnimation(rot_idx / 60.0f);
		}
		
		if(rotationCurve){
			rotationCurve->applyAnimation(rot_idx / 60.0f);
		}
		
		if(scaleCurve){
			scaleCurve->applyAnimation(rot_idx / 60.0f);
		}
		
		auto model = sfbx::as<sfbx::Model>(rotationCurve->getAnimationTarget());
		
		auto [t, r, s] = DecomposeTransform(SfbxMatToGlmMat(model->getLocalMatrix()));
		
		glm::vec3 rotationAxis = glm::degrees(glm::eulerAngles(r));
		
		push_position(t, rot_idx);
		push_rotation(r, rot_idx);
		push_scale(s, rot_idx);
	}
	
	
	for (int scale_idx = 0; scale_idx < num_scales; ++scale_idx)
	{
		if(positionCurve){
			positionCurve->applyAnimation(scale_idx / 60.0f);
		}
		
		if(rotationCurve){
			rotationCurve->applyAnimation(scale_idx / 60.0f);
		}
		
		if(scaleCurve){
			scaleCurve->applyAnimation(scale_idx / 60.0f);
		}
		
		auto model = sfbx::as<sfbx::Model>(rotationCurve->getAnimationTarget());
		
		auto [t, r, s] = DecomposeTransform(SfbxMatToGlmMat(model->getLocalMatrix()));
		
		push_position(t, scale_idx);
		push_rotation(r, scale_idx);
		push_scale(s, scale_idx);
	}
	
//	 unbind bind pose
	for (auto time : time_set_)
	{
		auto t = positions_.find(time);
		auto r = rotations_.find(time);
		auto s = scales_.find(time);
		glm::vec3 tt = (t != positions_.end()) ? t->second.position : glm::vec3(0.0f, 0.0f, 0.0f);
		glm::quat rr = (r != rotations_.end()) ? r->second.orientation : glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		glm::vec3 ss = (s != scales_.end()) ? s->second.scale : glm::vec3(1.0f, 1.0f, 1.0f);
		glm::mat4 transformation = glm::translate(glm::mat4(1.0f), tt) * glm::toMat4(glm::normalize(rr)) * glm::scale(glm::mat4(1.0f), ss);
		
		transformation = glm::inverse(this->get_bindpose()) * transformation;

		auto [translation, rotation, scale] = DecomposeTransform(transformation);
		if (t != positions_.end())
		{
			t->second.position = translation;
		}
		if (r != rotations_.end())
		{
			r->second.orientation = rotation;
		}
		if (s != scales_.end())
		{
			s->second.scale = scale;
		}
	}
}
void Bone::update(float animation_time, float factor)
{
	factor_ = factor;
	animation_time /= factor;
	glm::mat4 translation = interpolate_position(animation_time);
	glm::mat4 rotation = interpolate_rotation(animation_time);
	glm::mat4 scale = interpolate_scaling(animation_time);
	local_transform_ = translation * rotation * scale;
}

glm::mat4 &Bone::get_local_transform(float animation_time, float factor)
{
	update(animation_time, factor);
	return local_transform_;
}

const glm::mat4 &Bone::get_bindpose() const
{
	return bindpose_;
}

const std::set<float> &Bone::get_time_set() const
{
	return time_set_;
}

const std::string &Bone::get_name() const { return name_; }

float Bone::get_factor()
{
	return factor_;
}

void Bone::get_fbx_node(sfbx::Model* limb, sfbx::AnimationCurveNode* positionCurveNode, sfbx::AnimationCurveNode* rotationCurveNode, sfbx::AnimationCurveNode* scaleCurveNode, float factor, bool is_interpolated){
	
	std::map<float, glm::mat4> transform_map;
	transform_map[0.0f] = glm::mat4(1.0f);
	for (auto time : time_set_)
	{
		float timestamp = float(int(time * factor));
		transform_map[timestamp] = get_local_transform(timestamp, factor); // transformation;
	}
	
	if (is_interpolated && time_set_.size() > 0)
	{
		float duration = *time_set_.rbegin();
		float anim_time = 0.0f;
		std::map<float, glm::mat4> new_transform_map;
		auto before_transform = glm::mat4(1.0f);
		for (auto &t_mp : transform_map)
		{
			for (; anim_time < t_mp.first; anim_time += 1.0f)
			{
				new_transform_map[anim_time] = get_local_transform(anim_time, factor);
			}
			before_transform = new_transform_map[t_mp.first] = t_mp.second;
			anim_time = t_mp.first + 1.0f;
		}
		transform_map = new_transform_map;
	}
	for (auto &transform : transform_map)
	{
		auto transformation = get_bindpose() * transform.second;
		
		auto [t, r, s] = DecomposeTransform(transformation);

		float time = transform.first / 60.0f;

		positionCurveNode->addValue(time, sfbx::float3{ t.x, t.y, t.z });
		// Extract the rotation axis as a glm::vec3
		glm::vec3 eulerAngles = glm::eulerAngles(r);
		
		glm::vec3 eulerAnglesDeg = glm::degrees(glm::eulerAngles(r));
		
		rotationCurveNode->addValue(time, sfbx::float3{ eulerAnglesDeg.x, eulerAnglesDeg.y, eulerAnglesDeg.z});

		scaleCurveNode->addValue(time, sfbx::float3{ s.x, s.y, s.z });
	}
	
	if(transform_map.empty() || time_set_.empty()){
		auto transformation = SfbxMatToGlmMat(limb->getGlobalMatrix());
		
		auto [t, r, s] = DecomposeTransform(transformation);

		
		positionCurveNode->addValue(0, sfbx::float3{ t.x, t.y, t.z });
		// Extract the rotation axis as a glm::vec3
		glm::vec3 eulerAngles = glm::eulerAngles(r);
		
		glm::vec3 eulerAnglesDeg = glm::degrees(glm::eulerAngles(r));
		
		rotationCurveNode->addValue(0, sfbx::float3{ eulerAnglesDeg.x, eulerAnglesDeg.y, eulerAnglesDeg.z});
		
		scaleCurveNode->addValue(0, sfbx::float3{ s.x, s.y, s.z });
	}	
}
void Bone::set_name(const std::string &name)
{
	name_ = name;
}
void Bone::set_bindpose(const glm::mat4 &bindpose)
{
	bindpose_ = bindpose;
}
void Bone::push_position(const glm::vec3 &pos, float time, bool is_floor)
{
	if (is_floor && floor(time) != time)
	{
		return;
	}
	positions_[time] = {pos, time};
	time_set_.insert(time);
}
void Bone::push_rotation(const glm::quat &quat, float time, bool is_floor)
{
	if (is_floor && floor(time) != time)
	{
		return;
	}
	rotations_[time] = {quat, time};
	time_set_.insert(time);
}
void Bone::push_scale(const glm::vec3 &scale, float time, bool is_floor)
{
	if (is_floor && floor(time) != time)
	{
		return;
	}
	scales_[time] = {scale, time};
	time_set_.insert(time);
}

float Bone::get_scale_factor(float last_time_stamp, float next_time_stamp, float animation_time)
{
	float scaleFactor = 0.0f;
	float midWayLength = animation_time - last_time_stamp;
	float framesDiff = next_time_stamp - last_time_stamp;
	if (last_time_stamp == next_time_stamp || framesDiff < 0.0f)
	{
		return 1.0;
	}
	scaleFactor = midWayLength / framesDiff;
	return scaleFactor;
}

glm::mat4 Bone::interpolate_position(float animation_time)
{
	auto defaultPosition =
	KeyPosition{glm::vec3(0.0f, 0.0f, 0.0f), 0.0f};
	const auto &[p0Index, p1Index] = get_start_end<KeyPosition>(positions_, animation_time, defaultPosition);
	float scaleFactor = get_scale_factor(p0Index.get_time(),
										 p1Index.get_time(), animation_time);
	return glm::translate(glm::mat4(1.0f), glm::mix(p0Index.position, p1Index.position, scaleFactor));
}

glm::mat4 Bone::interpolate_rotation(float animation_time)
{
	auto defaultRotation = KeyRotation{glm::quat(1.0f, 0.0f, 0.0f, 0.0f), 0.0f};
	const auto &[p0Index, p1Index] = get_start_end<KeyRotation>(rotations_, animation_time, defaultRotation);
	float scaleFactor = get_scale_factor(p0Index.get_time(),
										 p1Index.get_time(), animation_time);
	glm::quat finalRotation = glm::slerp(p0Index.orientation, p1Index.orientation, scaleFactor);
	return glm::toMat4(glm::normalize(finalRotation));
}

glm::mat4 Bone::interpolate_scaling(float animation_time)
{
	auto defaultScale = KeyScale{glm::vec3(1.0f, 1.0f, 1.0f), 0.0f};
	
	const auto &[p0Index, p1Index] = get_start_end<KeyScale>(scales_, animation_time, defaultScale);
	
	float scaleFactor = get_scale_factor(p0Index.get_time(),
										 p1Index.get_time(), animation_time);
	return glm::scale(glm::mat4(1.0f), glm::mix(p0Index.scale, p1Index.scale, scaleFactor));
}

void Bone::replace_or_add_keyframe(const glm::mat4 &transform, float time, bool ignoreEpsilon)
{
	std::tuple<glm::vec3, glm::quat, glm::vec3> ltrs;
	
	if(ignoreEpsilon){
		ltrs = DecomposeTransform(transform);
	} else {
		ltrs = DecomposeTransform(get_local_transform(time, factor_));
	}
	
	auto [lt, lr, ls] = ltrs;
	
	float time_stamp = floorf(time) / factor_;
	auto [t, r, s] = DecomposeTransform(transform);
	
	bool is_t_changed = !(lt.x - 1e-3 < t.x && t.x < lt.x + 1e-3 && lt.y - 1e-3 < t.y && t.y < lt.y + 1e-3 && lt.z - 1e-3 < t.z && t.z < lt.z + 1e-3);
	bool is_r_changed = !(lr.x - 1e-3 < r.x && r.x < lr.x + 1e-3 && lr.y - 1e-3 < r.y && r.y < lr.y + 1e-3 && lr.z - 1e-3 < r.z && r.z < lr.z + 1e-3);
	bool is_s_changed = !(ls.x - 1e-3 < s.x && s.x < ls.x + 1e-3 && ls.y - 1e-3 < s.y && s.y < ls.y + 1e-3 && ls.z - 1e-3 < s.z && s.z < ls.z + 1e-3);
	bool is_add = false;
	
	// replace
	if (positions_.find(time_stamp) != positions_.end() || is_t_changed || positions_.size() == 0)
	{
		is_add = true;
		positions_[time_stamp] = {t, time_stamp};
	}
	if (rotations_.find(time_stamp) != rotations_.end() || is_r_changed || rotations_.size() == 0)
	{
		is_add = true;
		rotations_[time_stamp] = {r, time_stamp};
	}
	if (scales_.find(time_stamp) != scales_.end() || is_s_changed || scales_.size() == 0)
	{
		is_add = true;
		scales_[time_stamp] = {s, time_stamp};
	}
	if (is_add)
	{
		time_set_.insert(time_stamp);
	}
}

void Bone::replace_or_sub_keyframe(const glm::mat4 &transform, float time)
{
	float time_stamp = floorf(time) / factor_;
	auto [t, r, s] = DecomposeTransform(transform);
	auto [it_t, it_r, it_s] = std::tuple{positions_.find(time_stamp),
		rotations_.find(time_stamp),
		scales_.find(time_stamp)};
	sub_keyframe(time);
	
	auto erased_transform = get_local_transform(time, factor_);
	auto [lt, lr, ls] = DecomposeTransform(erased_transform);
	float tolerance = 1e-3;
	bool is_t_changed = !(lt.x - tolerance < t.x && t.x < lt.x + tolerance && lt.y - tolerance < t.y && t.y < lt.y + tolerance && lt.z - tolerance < t.z && t.z < lt.z + tolerance);
	bool is_r_changed = !(lr.x - tolerance < r.x && r.x < lr.x + tolerance && lr.y - tolerance < r.y && r.y < lr.y + tolerance && lr.z - tolerance < r.z && r.z < lr.z + tolerance);
	bool is_s_changed = !(ls.x - tolerance < s.x && s.x < ls.x + tolerance && ls.y - tolerance < s.y && s.y < ls.y + tolerance && ls.z - tolerance < s.z && s.z < ls.z + tolerance);
	if (is_t_changed || is_r_changed || is_s_changed || time_stamp == 0.0f)
	{
		replace_or_add_keyframe(transform, time);
	}
}

bool Bone::sub_keyframe(float time, bool is_animation_time)
{
	float time_stamp = floorf(time) / factor_;
	if (is_animation_time)
	{
		time_stamp = time;
	}
	auto [it_t, it_r, it_s] = std::tuple{positions_.find(time_stamp),
		rotations_.find(time_stamp),
		scales_.find(time_stamp)};
	bool is_erased = false;
	if (it_t != positions_.end())
	{
		is_erased = true;
		positions_.erase(it_t);
	}
	if (it_r != rotations_.end())
	{
		is_erased = true;
		rotations_.erase(it_r);
	}
	if (it_s != scales_.end())
	{
		is_erased = true;
		scales_.erase(it_s);
	}
	time_set_.erase(time_stamp);
	
	return is_erased;
}
}
