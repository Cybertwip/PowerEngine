#include "animator.h"

#include <glm/gtx/matrix_decompose.hpp>
#include "entity.h"
#include "bone.h"
#include "../util/utility.h"
#include "shader.h"
#include "../entity/components/animation_component.h"
#include "../entity/components/renderable/mesh_component.h"
#include "../entity/components/renderable/armature_component.h"
#include "../entity/components/pose_component.h"
#include "animation.h"
#include "json_animation.h"
#include "assimp_animation.h"
#include "shared_resources.h"

#include "scene/scene.hpp"
#include "UI/ui_context.h"

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <iostream>

namespace {


std::vector<std::shared_ptr<anim::StackedAnimation>> flattenAndSortAnimations(const std::vector<std::vector<std::shared_ptr<anim::StackedAnimation>>>& tracks) {
	std::vector<std::shared_ptr<anim::StackedAnimation>> allAnimations;
	// Flatten all animations into a single vector
	for (const auto& track : tracks) {
		allAnimations.insert(allAnimations.end(), track.begin(), track.end());
	}
	// Sort all animations by their start time
	std::sort(allAnimations.begin(), allAnimations.end(), [](const std::shared_ptr<anim::StackedAnimation>& a, const std::shared_ptr<anim::StackedAnimation>& b) {
		return a->get_start_time() < b->get_start_time();
	});
	return allAnimations;
}

std::shared_ptr<anim::StackedAnimation> findEarliestAnimation(const std::vector<std::vector<std::shared_ptr<anim::StackedAnimation>>>& tracks) {
	std::shared_ptr<anim::StackedAnimation> earliestAnimation = nullptr;
	int earliestStartTime = std::numeric_limits<int>::max();
	
	for (const auto& track : tracks) {
		for (const auto& anim : track) {
			int startTime = anim->get_start_time();
			if (startTime < earliestStartTime) {
				earliestAnimation = anim;
				earliestStartTime = startTime;
			}
		}
	}
	
	return earliestAnimation;
}

float calculate_blend_factor_for_current_animation(const std::shared_ptr<anim::StackedAnimation>& prevAnim, const std::shared_ptr<anim::StackedAnimation>& currentAnim, int currentTime) {
	
	if(currentTime == -1){
		return 1.0f;
	}
	
	int startA = currentAnim->get_start_time();
	int startB = prevAnim != nullptr ? prevAnim->get_start_time() : 0;
	int endA = currentAnim->get_end_time();
	int endB = prevAnim != nullptr ? prevAnim->get_end_time() : 0;
	float weightA = currentAnim->get_weight();
	float weightB = prevAnim != nullptr ? prevAnim->get_weight() : 0;
	
	
	bool isAnimAFirst = startA <= startB;
	
	float blendFactor = 1.0f;
	
	if(prevAnim == nullptr) {
		// Case 1: No Previous Animation
		blendFactor = 1.0f; // Current animation takes full effect
	} else {
		// There is a previous animation, calculate overlap
		int overlapStart = std::max(startA, startB);
		int overlapEnd = std::min(endA, endB);
		
		
		// Adjust for non-overlapping cases where one animation is within the other
		if (startA >= startB && endA <= endB) {
			// Current animation (A) is fully within previous animation (B)
			if (currentTime >= startA && currentTime <= endA) {
				blendFactor = (currentTime - startB) / (float)(endB - startB);
				blendFactor *= 0.5f;
			} else {
				blendFactor = isAnimAFirst ? 1.0f : 0.0f; // Full effect to the one that starts first
			}
		} else if (startB >= startA && endB <= endA) {
			// Current animation (A) is fully within previous animation (B)
			if (currentTime >= startB && currentTime <= endB) {
				blendFactor = (currentTime - startA) / (float)(endA - startA);
				blendFactor *= 0.5f;
			} else {
				blendFactor = isAnimAFirst ? 1.0f : 0.0f; // Full effect to the one that starts first
			}
		} else {
			
			if(currentTime < overlapStart) {
				// Before overlap starts
				blendFactor = isAnimAFirst ? 1.0f : 0.0f; // Full effect to the one that starts first
			} else if(currentTime > overlapEnd) {
				// After overlap ends
				blendFactor = endA > endB ? 1.0f : 0.0f; // Full effect to the one that ends last
			} else {
				// Within overlap period
				float totalOverlap = static_cast<float>(overlapEnd - overlapStart);
				float progress = (currentTime - overlapStart) / totalOverlap; // Normalized [0, 1]
				
				blendFactor = progress;
			}
			
		}
		
	}
	
	return blendFactor;
}

glm::mat4 cubicInterpolation(const glm::mat4& startMatrix, const glm::mat4& endMatrix, float blendFactor) {
	float t = blendFactor;
	float t2 = t * t;
	float t3 = t2 * t;
	float mt = 1.0f - t;
	float mt2 = mt * mt;
	float mt3 = mt2 * mt;
	
	glm::mat4 result;
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			float p0 = startMatrix[i][j];
			float p1 = endMatrix[i][j];
			result[i][j] = mt3 * p0 + 3.0f * mt2 * t * p0 + 3.0f * mt * t2 * p1 + t3 * p1;
		}
	}
	return result;
}

glm::quat Slerp(const glm::quat& quaternion1, const glm::quat& quaternion2, float amount) {
	const float epsilon = 1e-6f;
	float t = amount;
	
	float cosOmega = glm::dot(quaternion1, quaternion2);
	
	bool flip = false;
	
	if (cosOmega < 0.0f) {
		flip = true;
		cosOmega = -cosOmega;
	}
	
	float s1, s2;
	
	if (cosOmega > (1.0f - epsilon)) {
		// Too close, do straight linear interpolation.
		s1 = 1.0f - t;
		s2 = flip ? -t : t;
	} else {
		float omega = std::acos(cosOmega);
		float invSinOmega = 1.0f / std::sin(omega);
		
		s1 = std::sin((1.0f - t) * omega) * invSinOmega;
		s2 = flip ? -std::sin(t * omega) * invSinOmega
		: std::sin(t * omega) * invSinOmega;
	}
	
	glm::quat ans;
	
	ans.x = s1 * quaternion1.x + s2 * quaternion2.x;
	ans.y = s1 * quaternion1.y + s2 * quaternion2.y;
	ans.z = s1 * quaternion1.z + s2 * quaternion2.z;
	ans.w = s1 * quaternion1.w + s2 * quaternion2.w;
	
	return ans;
}

void EnforceShortestArcWith(glm::quat& FirstQuat, const glm::quat& SecondQuat)
{
	const float DotResult = glm::dot(SecondQuat, FirstQuat);
	const float Bias = DotResult >= 0 ? 1.0f : -1.0f;
	
	FirstQuat *= Bias;
}
}

namespace anim
{

void printUpAxis(const glm::mat4& mat) {
	// Extract UP vector (local Y-axis) from the transformation matrix
	glm::vec3 upAxis = glm::normalize(glm::vec3(mat[1])); // Using the second column
	// Print UP axis
	std::cout << "UP axis: (" << upAxis.x << ", " << upAxis.y << ", " << upAxis.z << ")" << std::endl;
}

void BlendTransforms(const std::map<int, Keyframe>& transformStack, int currentTime, glm::mat4& outputTransform) {
	if (transformStack.empty()) return;
	
	auto it = transformStack.lower_bound(currentTime);
	if (it == transformStack.begin()) {
		// If currentTime is at or before the first keyframe, use the first directly
		outputTransform = it->second.transform;
		return;
	} else if (it == transformStack.end()) {
		// If currentTime is beyond the last keyframe, use the last directly
		--it;
		outputTransform = it->second.transform;
		return;
	}
	
	auto nextIt = it; // The found or next higher keyframe
	auto prevIt = std::prev(it); // The previous keyframe
	
	// If exact match or only one valid next, use it directly without blending
	if (currentTime == it->first || prevIt == transformStack.end()) {
		outputTransform = nextIt->second.transform;
		return;
	}
	
	// Calculate blend factor based on currentTime's position between the two keyframes
	int nextTime = nextIt->first;
	int prevTime = prevIt->first;
	float blendFactor = (currentTime - prevTime) / static_cast<float>(nextTime - prevTime);
	
	// Ensure blendFactor is clamped between 0 and 1
	blendFactor = glm::clamp(blendFactor, 0.0f, 1.0f);
	
	glm::vec3 startPos, startScale, endPos, endScale;
	glm::quat startRot, endRot;
	glm::vec3 skew;
	glm::vec4 perspective;
	
	glm::decompose(prevIt->second.transform, startScale, startRot, startPos, skew, perspective);
	glm::decompose(nextIt->second.transform, endScale, endRot, endPos, skew, perspective);
	
	// Linearly interpolate (lerp) position, yaw, and pitch between prevIt and nextIt
	auto blendedPos = glm::mix(startPos, endPos, blendFactor);
	auto blendedRot = glm::slerp(startRot, endRot, blendFactor); // Use slerp for quaternion rotation
	auto blendedScale = glm::mix(startScale, endScale, blendFactor);
	
	glm::mat4 blendedMat = glm::translate(glm::mat4(1.0f), blendedPos)
	* glm::mat4_cast(glm::normalize(blendedRot))
	* glm::scale(glm::mat4(1.0f), blendedScale);
	outputTransform = blendedMat;
}

glm::mat4 interpolateTransformations(const glm::mat4& start, const glm::mat4& end, float blendFactor, bool rootBone = false) {
	// Decompose start matrix
	glm::vec3 startPos, startScale, endPos, endScale;
	glm::quat startRot, endRot;
	glm::vec3 skew;
	glm::vec4 perspective;
	
	glm::decompose(start, startScale, startRot, startPos, skew, perspective);
	glm::decompose(end, endScale, endRot, endPos, skew, perspective);
	
	// Interpolate position, rotation, and scale
	glm::vec3 blendedPos = glm::mix(startPos, endPos, blendFactor);
	glm::vec3 blendedScale = glm::mix(startScale, endScale, blendFactor);
	
	// Perform SLERP
	glm::quat blendedRot;
	blendedRot = Slerp(startRot, endRot, blendFactor);
	
	// Recompose the blended matrix
	glm::mat4 blendedMat = glm::translate(glm::mat4(1.0f), blendedPos)
	* glm::mat4_cast(glm::normalize(blendedRot))
	* glm::scale(glm::mat4(1.0f), blendedScale);
	return blendedMat;
}

struct AnimationOverlap {
	glm::mat4 local;
	float blend;
};

int adjustWrappedTime(int currentTime, int start_time, int end_time, int offset, int duration) {
	int wrapped_time = currentTime;
	if (wrapped_time != -1) {
		if (offset < 0 && wrapped_time + offset < start_time + offset) {
			wrapped_time = duration - std::fabs(std::fmod(wrapped_time - start_time, duration));
		} else {
			wrapped_time = std::fmod(wrapped_time - start_time, duration);
		}
		
		if (wrapped_time > end_time) {
			wrapped_time = start_time;
		}
		
	}
	return wrapped_time;
}

Animator::Animator(std::shared_ptr<SharedResources> resources)
: is_stop_(true), _resources(resources)
{
	final_bone_matrices_.reserve(MAX_BONE_NUM);
	for (unsigned int i = 0U; i < MAX_BONE_NUM; i++)
		final_bone_matrices_.push_back(glm::mat4(1.0f));
}

void Animator::update(float dt)
{
	float end_time = end_time_;
	
	if (mode_ == AnimatorMode::Sequence) {
		end_time = sequencer_end_time_;
	} else if (mode_ == AnimatorMode::Animation) {
		if (auto animationSequence = std::dynamic_pointer_cast<AnimationSequence>(active_animation_sequence_); animationSequence) {
			end_time = animationSequence->get_sequencer().GetFrameMax();
		}
	}
	
	if (is_export_) {
		if (!is_stop_) {
			current_time_ += 1;
		}
	} else {
		if (!is_stop_) {
			current_time_ += fps_ * dt * direction_;
			current_time_ = fmax(start_time_, fmod(end_time + current_time_, end_time));
		} else {
			current_time_ = floor(current_time_);
		}
	}
	
	if (mode_ == AnimatorMode::Map) {
		// Calculate currentTime outside of the loop
		float start_time = 0;
		float end_time = 0;
		float currentTime = 0;
		
		for (auto& runtimeSequence : runtime_cinematic_sequences_) {
			auto& sequence = runtimeSequence->get_sequence();
			
			auto entityId = sequence.mEntityId;
			
			auto entity = _resources->get_entity(entityId);
			
			if (!entity) {
				continue;
			}
			
			auto pose = entity->get_component<PoseComponent>();
			auto root = pose->get_root_entity();
			auto animation = pose->get_animation_component();
			auto& sequencer = runtimeSequence->get_sequencer();
			
			if (animation->get_animation_stack_tracks().empty()) {
				animation->set_current_time(-1);
			} else {
				end_time = sequencer.mFrameMax;
				currentTime = animation->get_current_time();
				
				// Update currentTime
				if (!is_stop_) {
					currentTime += fps_ * dt * direction_;
					currentTime = fmax(start_time, fmod(end_time + currentTime, end_time));
				} else {
					currentTime = floor(currentTime);
				}
				
				animation->set_current_time(currentTime);
			}
		}
	} else if (mode_ == AnimatorMode::Animation) {
		if (auto animationSequence = std::dynamic_pointer_cast<AnimationSequence>(active_animation_sequence_); animationSequence) {
			
			auto entity = animationSequence->get_root();
			
			if (entity) {
				auto pose = entity->get_component<PoseComponent>();
				auto root = pose->get_root_entity();
				auto animation = pose->get_animation_component();
				
				float start_time = 0;
				float end_time = animation->get_custom_duration();
				float currentTime = animation->get_current_time();
				
				if (!is_stop_) {
					currentTime += dt * direction_;
					currentTime = fmax(start_time, fmod(end_time + currentTime, end_time));
				} else {
					currentTime = floor(currentTime);
				}
				
				animation->set_current_time(currentTime);
			}
		}
	}
}


void Animator::update_sequencer(Scene& scene, ui::UiContext& ui_context){
	if(mode_ == AnimatorMode::Sequence && active_cinematic_sequence_){
		auto& sequencer = static_cast<TracksSequencer&>(active_cinematic_sequence_->get_sequencer());
		
		for(auto& item : sequencer.mItems){
			auto entityId = item->mId;
			
			auto entity = scene.get_camera(entityId);
			
			if(!entity){
				continue;
			}
			
			for(auto track : item->mTracks){
				if(track->mType == ItemType::Transform){
					
					auto camera = entity->get_component<CameraComponent>();
					
					glm::vec3 euler_angles = glm::radians(glm::vec3(camera->get_pitch(), camera->get_yaw(), 0.0f));
					glm::quat camera_rotation = glm::quat(euler_angles);
					
					glm::mat4 transform = ComposeTransform(camera->get_position(), camera_rotation, glm::vec3(1.0f, 1.0f, 1.0f));
					
					int currentTime = static_cast<int>(get_current_frame_time());
					
					auto transformStack = track->mKeyFrames;
					
					BlendTransforms(transformStack, currentTime, transform);
					
					bool hovered = ui_context.scene.is_focused;
					
					if(!hovered){
						entity->set_local(transform);
					}
				} else {
					continue;
				}
			}
			
		}
		for(auto item : sequencer.mItems){
			auto entityId = item->mId;
			
			auto entity = scene.get_light(entityId);
			
			if(!entity){
				continue;
			}
			
			for(auto track : item->mTracks){
				if(track->mType == ItemType::Transform){
					auto component = entity->get_component<DirectionalLightComponent>();
					
					auto transform = entity->get_local();
					
					int currentTime = static_cast<int>(get_current_frame_time());
					
					auto transformStack = track->mKeyFrames;
					
					BlendTransforms(transformStack, currentTime, transform);
					
					bool hovered = ui_context.scene.is_focused;
					
					if(!hovered){
						entity->set_local(transform);
					}
				} else {
					continue;
				}
			}
		}
		
		for(auto item : sequencer.mItems){
			auto entityId = item->mId;
			
			auto entity = _resources->get_entity(entityId);
			
			if(!entity || entity->get_component<CameraComponent>() || entity->get_component<DirectionalLightComponent>()){
				continue;
			}
			
			auto pose = entity->get_component<PoseComponent>();
			auto animation = pose->get_animation_component();
			auto root = pose->get_root_entity();
			auto shader = pose->get_shader();
			
			assert(animation && root && shader);
			
			for(auto track : item->mTracks){
				if(track->mType == ItemType::Transform){
					
					auto& transformStack = track->mKeyFrames;
					
					auto mutable_animation = animation->get_animation();
					
					if(mutable_animation){
						for(auto& transform : transformStack){
							mutable_animation->add_and_replace_bone(entity->get_name(), transform.second.transform, transform.first);
						}
					}
					
					if(!transformStack.empty()){
						calculate_root_transform(entity, animation, glm::mat4(1.0f));
					}
					
				} else {
					
					int currentTime = static_cast<int>(get_current_frame_time());
					
					if(animation->get_animation_stack_tracks().empty()){
						animation->set_current_time(-1);
					} else {
						animation->set_current_time(get_current_frame_time());
					}
					std::vector<std::vector<glm::mat4>> blendedMatrices;
					
					// Ensure there's at least one animation track to process
					int blendIndex = 0;
					
					
					std::vector<glm::mat4> accumulatedResult(MAX_BONE_NUM, glm::mat4(1.0f)); // Initialize with identity matrices
					// Adjust the initialization of accumulatedResult to use the first-most animation's transformations
					if (!animation->get_animation_stack_tracks().empty()) {
						// Find the earliest animation across all tracks
						auto earliestAnimation = findEarliestAnimation(animation->get_animation_stack_tracks());
						
						if (earliestAnimation) {
							// Calculate transformation for the earliest animation and use it as the base for blending
							calculate_bone_transform(root, animation, earliestAnimation, glm::mat4(1.0f));
							accumulatedResult = final_bone_matrices_; // Use these matrices as the initial accumulatedResult
						} else {
							// If no animations are found, initialize with identity matrices
							for (unsigned int i = 0; i < MAX_BONE_NUM; i++) {
								accumulatedResult[i] = glm::mat4(1.0f);
							}
						}
						
						size_t blendIndex = 0;
						
						auto allAnimations = flattenAndSortAnimations(animation->get_animation_stack_tracks());
						
						for (size_t i = 0; i < allAnimations.size(); ++i) {
							auto& currentAnim = allAnimations[i];
							
							std::shared_ptr<StackedAnimation> prevAnim = (i > 0) ? allAnimations[i - 1] : nullptr;
							
							// Calculate transformation for the current animation
							calculate_bone_transform(root, animation, currentAnim, glm::mat4(1.0f));
							std::vector<glm::mat4> currentAnimMatrices = final_bone_matrices_;
							
							// Determine the blend factor for the current animation
							// Adjust as necessary to accommodate your specific logic for calculating blend factors
							float blendFactor = calculate_blend_factor_for_current_animation(prevAnim, currentAnim, currentTime);
							
							// Blend the current animation into the accumulated result
							for (size_t boneIndex = 0; boneIndex < accumulatedResult.size(); ++boneIndex) {
								glm::mat4 baseMatrix = (i == 0) ? accumulatedResult[boneIndex] : accumulatedResult[boneIndex];
								glm::mat4 animMatrix = currentAnimMatrices[boneIndex];
								
								// Blend and update the accumulated result with the current animation's contribution
								
								if(boneIndex == root->get_component<ArmatureComponent>()->get_id()){
									accumulatedResult[boneIndex] = interpolateTransformations(baseMatrix, animMatrix, blendFactor, true);
									
								} else {
									accumulatedResult[boneIndex] = interpolateTransformations(baseMatrix, animMatrix, blendFactor);
								}
							}
							
						}
						
						// The accumulatedResult now contains the blended transformations of all animations in the stack
						final_bone_matrices_ = accumulatedResult;
						
						// Apply any final adjustments needed after blending all animations in the stack
						apply_bone_offsets(root, glm::mat4(1.0f));
					} else {
						final_bone_matrices_ = accumulatedResult;
						apply_bone_offsets(root, glm::mat4(1.0f));
					}
					
					
					root->set_local(accumulatedResult[root->get_component<ArmatureComponent>()->get_id()]);

					animation->update_matrices(final_bone_matrices_);
				}
			}
		}
	} else
		if(mode_ == AnimatorMode::Map){
			for(auto& runtimeSequence : runtime_cinematic_sequences_){
				auto& sequence = runtimeSequence->get_sequence();
				auto& sequencer = runtimeSequence->get_sequencer();
							
				for(auto& item : sequencer.mItems){
					auto entityId = item->mId;
					
					auto entity = scene.get_camera(entityId);
					
					if(!entity){
						continue;
					}
					
					for(auto track : item->mTracks){
						if(track->mType == ItemType::Transform){
							
							auto camera = entity->get_component<CameraComponent>();
							
							glm::vec3 euler_angles = glm::radians(glm::vec3(camera->get_pitch(), camera->get_yaw(), 0.0f));
							glm::quat camera_rotation = glm::quat(euler_angles);
							
							glm::mat4 transform = ComposeTransform(camera->get_position(), camera_rotation, glm::vec3(1.0f, 1.0f, 1.0f));
							
							int currentTime = static_cast<int>(get_current_frame_time());
							
							auto transformStack = track->mKeyFrames;
							
							BlendTransforms(transformStack, currentTime, transform);
							
							bool hovered = ui_context.scene.is_focused;
							
							if(!hovered){
								entity->set_local(transform);
							}
						} else {
							continue;
						}
					}
					
				}
				for(auto item : sequencer.mItems){
					auto entityId = item->mId;
					
					auto entity = scene.get_light(entityId);
					
					if(!entity){
						continue;
					}
					
					for(auto track : item->mTracks){
						if(track->mType == ItemType::Transform){
							auto component = entity->get_component<DirectionalLightComponent>();
							
							auto transform = entity->get_local();
							
							int currentTime = static_cast<int>(get_current_frame_time());
							
							auto transformStack = track->mKeyFrames;
							
							BlendTransforms(transformStack, currentTime, transform);
							
							bool hovered = ui_context.scene.is_focused;
							
							if(!hovered){
								entity->set_local(transform);
							}
						} else {
							continue;
						}
					}
				}
				
				for(auto item : sequencer.mItems){
					auto entityId = item->mId;
					
					auto entity = _resources->get_entity(entityId);
					
					if(!entity || entity->get_component<CameraComponent>() || entity->get_component<DirectionalLightComponent>()){
						continue;
					}
					
					auto pose = entity->get_component<PoseComponent>();
					auto animation = pose->get_animation_component();
					auto root = pose->get_root_entity();
					auto shader = pose->get_shader();
					
					assert(animation && root && shader);
					
					for(auto track : item->mTracks){
						if(track->mType == ItemType::Transform){
							
							auto& transformStack = track->mKeyFrames;
							
							auto mutable_animation = animation->get_animation();
							
							if(mutable_animation){
								for(auto& transform : transformStack){
									mutable_animation->add_and_replace_bone(entity->get_name(), transform.second.transform, transform.first);
								}
							}
							
							if(!transformStack.empty()){
								calculate_root_transform(entity, animation, glm::mat4(1.0f));
							}
							
						} else {
							
							int currentTime = static_cast<int>(animation->get_current_time());
							
							std::vector<std::vector<glm::mat4>> blendedMatrices;
							
							// Ensure there's at least one animation track to process
							int blendIndex = 0;
							
							
							std::vector<glm::mat4> accumulatedResult(MAX_BONE_NUM, glm::mat4(1.0f)); // Initialize with identity matrices
							// Adjust the initialization of accumulatedResult to use the first-most animation's transformations
							if (!animation->get_animation_stack_tracks().empty()) {
								// Find the earliest animation across all tracks
								auto earliestAnimation = findEarliestAnimation(animation->get_animation_stack_tracks());
								
								if (earliestAnimation) {
									// Calculate transformation for the earliest animation and use it as the base for blending
									calculate_bone_transform(root, animation, earliestAnimation, glm::mat4(1.0f));
									accumulatedResult = final_bone_matrices_; // Use these matrices as the initial accumulatedResult
								} else {
									// If no animations are found, initialize with identity matrices
									for (unsigned int i = 0; i < MAX_BONE_NUM; i++) {
										accumulatedResult[i] = glm::mat4(1.0f);
									}
								}
								
								size_t blendIndex = 0;
								
								auto allAnimations = flattenAndSortAnimations(animation->get_animation_stack_tracks());
								
								for (size_t i = 0; i < allAnimations.size(); ++i) {
									auto& currentAnim = allAnimations[i];
									
									std::shared_ptr<StackedAnimation> prevAnim = (i > 0) ? allAnimations[i - 1] : nullptr;
									
									// Calculate transformation for the current animation
									calculate_bone_transform(root, animation, currentAnim, glm::mat4(1.0f));
									std::vector<glm::mat4> currentAnimMatrices = final_bone_matrices_;
									
									// Determine the blend factor for the current animation
									// Adjust as necessary to accommodate your specific logic for calculating blend factors
									float blendFactor = calculate_blend_factor_for_current_animation(prevAnim, currentAnim, currentTime);
									
									// Blend the current animation into the accumulated result
									for (size_t boneIndex = 0; boneIndex < accumulatedResult.size(); ++boneIndex) {
										glm::mat4 baseMatrix = (i == 0) ? accumulatedResult[boneIndex] : accumulatedResult[boneIndex];
										glm::mat4 animMatrix = currentAnimMatrices[boneIndex];
										
										// Blend and update the accumulated result with the current animation's contribution
										
										if(boneIndex == root->get_component<ArmatureComponent>()->get_id()){
											accumulatedResult[boneIndex] = interpolateTransformations(baseMatrix, animMatrix, blendFactor, true);
											
										} else {
											accumulatedResult[boneIndex] = interpolateTransformations(baseMatrix, animMatrix, blendFactor);
										}
									}
									
								}
								
								// The accumulatedResult now contains the blended transformations of all animations in the stack
								final_bone_matrices_ = accumulatedResult;
								
								// Apply any final adjustments needed after blending all animations in the stack
								apply_bone_offsets(root, glm::mat4(1.0f));
							} else {
								final_bone_matrices_ = accumulatedResult;
								apply_bone_offsets(root, glm::mat4(1.0f));
							}
							
							
							root->set_local(accumulatedResult[root->get_component<ArmatureComponent>()->get_id()]);
							
							animation->update_matrices(final_bone_matrices_);
						}
					}
				}
				
			}
		} else if(mode_ == AnimatorMode::Animation){
			if(active_animation_sequence_){
				
				auto sequence = std::dynamic_pointer_cast<AnimationSequence>(active_animation_sequence_);

				auto& sequencer = static_cast<AnimationSequencer&>(active_animation_sequence_->get_sequencer());
				
				for(auto& track : sequencer.mItems){
					auto entityId = track->mId;
					//@TODO _resources->get_scene()
					auto entity = _resources->get_entity(entityId);
					
					if(!entity){
						continue;
					}
					
					auto component = entity->get_component<TransformComponent>();
					
					auto transform = component->get_mat4();
					
					int currentTime = static_cast<int>(get_current_frame_time());
					
					auto transformStack = track->mKeyFrames;
					
					BlendTransforms(transformStack, currentTime, transform);
					
					bool hovered = ui_context.scene.is_focused;
					
					if(!hovered){
						entity->set_local(transform);
					}
				}
				
				
				auto entity = sequence->get_root();
				
				auto pose = entity->get_component<PoseComponent>();
				auto animationComponent = pose->get_animation_component();
				auto root = pose->get_root_entity();
				auto shader = pose->get_shader();
				
				assert(animationComponent && root && shader);
				
				const auto& animation = _resources->get_animation(sequence->get_animation_id());

				
				animationComponent->set_current_time(get_current_frame_time());
				
				//@TODO optimize
				auto currentAnim = std::make_shared<StackedAnimation>(animation, 0, animation->get_duration(), 0);
				
				calculate_bone_transform(root, animationComponent, currentAnim, glm::mat4(1.0f));
				
				root->set_local(final_bone_matrices_[root->get_component<ArmatureComponent>()->get_id()]);
				apply_bone_offsets(root, glm::mat4(1.0f));
				
				animationComponent->update_matrices(final_bone_matrices_);
				
			}
		}
}

void Animator::update_animation(std::shared_ptr<AnimationComponent> animation, std::shared_ptr<Entity> root, Shader *shader)
{
	assert(animation && shader);
	auto& baked_matrices = animation->get_matrices();
	shader->use();
	for (int i = 0; i < MAX_BONE_NUM; ++i)
	{
		shader->set_mat4("finalBonesMatrices[" + std::to_string(i) + "]", baked_matrices[i]);
	}
}

void Animator::apply_bone_offsets(std::shared_ptr<Entity> entity, const glm::mat4 &parentTransform)
{
	glm::mat4 global_transformation = parentTransform;
	
	auto armature = entity->get_component<ArmatureComponent>();
	
	if(armature){
		global_transformation *= armature->get_bindpose();
		
		int id = armature->get_id();
		if (id < MAX_BONE_NUM) {
			global_transformation *= final_bone_matrices_[id];
			
			final_bone_matrices_[id] = global_transformation * armature->get_bone_offset();
			
			armature->set_model_pose(global_transformation);
			
		}
	}
	
	for (auto& child : entity->get_mutable_children()) {
		apply_bone_offsets(child, global_transformation);
	}
}

void Animator::calculate_bone_transform(std::shared_ptr<Entity> entity, std::shared_ptr<AnimationComponent> animationComponent, std::shared_ptr<StackedAnimation> currentAnimation, const glm::mat4 &parentTransform)
{
	const std::string &node_name = entity->get_name();
	glm::mat4 global_transformation = parentTransform;
	entity->set_local(glm::mat4(1.0f));
	
	auto armature = entity->get_component<ArmatureComponent>();
	
	if(armature){
		global_transformation *= armature->get_bindpose();
	}
	
	glm::mat4 blendedLocal = glm::identity<glm::mat4>();
	int currentFrame = static_cast<int>(animationComponent->get_current_time());
	
	int offset = currentAnimation->get_offset();
	int start_time = currentAnimation->get_start_time() - offset;
	float end_time = currentAnimation->get_end_time();
	int duration = currentAnimation->get_duration();
	int wrapped_time = adjustWrappedTime(animationComponent->get_current_time(), start_time, end_time, offset, duration);
	
	if(currentFrame >= currentAnimation->get_start_time() && currentFrame <= end_time) {
		auto bone = currentAnimation->get_wrapped_animation()->find_bone(node_name);
		if (bone != nullptr && bone->get_time_set().size() > 0) {
			auto local = bone->get_local_transform(wrapped_time, 1);
			if (entity->get_mutable_parent()->get_component<ArmatureComponent>() == nullptr && mIsRootMotion) {
				local = glm::mat4(glm::mat3(local));
			}
			blendedLocal = local;
		}
	}
	entity->set_local(blendedLocal);
	global_transformation *= entity->get_local();
	
	if(armature){
		int id = armature->get_id();
		if (id < MAX_BONE_NUM) {
			final_bone_matrices_[id] = blendedLocal;
		}
	}
	
	for (auto& child : entity->get_mutable_children()) {
		calculate_bone_transform(child, animationComponent, currentAnimation, global_transformation);
	}
}


void Animator::calculate_root_transform(std::shared_ptr<Entity> entity, std::shared_ptr<AnimationComponent> animationComponent, const glm::mat4 &parentTransform)
{
	const std::string &node_name = entity->get_name();
	glm::mat4 global_transformation = parentTransform;
	entity->set_local(glm::mat4(1.0f));
	
	// 바인딩 포즈
	auto armature = entity->get_component<ArmatureComponent>();
	
	if(armature){
		global_transformation *= armature->get_bindpose();
	}
	
	auto animation = animationComponent->get_animation();
	if (animation)
	{
		auto bone = animation->find_bone(node_name);
		if (bone != nullptr && bone->get_time_set().size() > 0)
		{
			auto local = bone->get_local_transform(animationComponent->get_current_time(), 1);
			if (entity->get_mutable_parent()->get_component<ArmatureComponent>() == nullptr && mIsRootMotion)
			{
				local = glm::mat4(glm::mat3(local));
			}
			
			
			entity->set_local(local);
		}
		
	}
	
}

void Animator::calculate_mesh_transform(std::shared_ptr<Entity> entity, std::shared_ptr<AnimationComponent> animationComponent,  std::vector<std::shared_ptr<StackedAnimation>>& animationStack, const glm::mat4 &parentTransform)
{
	const std::string &node_name = entity->get_name();
	glm::mat4 global_transformation = parentTransform;
	
	glm::mat4 blendedLocal = glm::identity<glm::mat4>();
	
	bool modified = false;
	
	float previousIntersectionTime = 0;
	float previousDuration = 0;
	
	float previousStartTime = 0;
	int previousEndTime = 0;
	
	for (auto it = animationStack.begin(); it != animationStack.end(); ++it)
	{
		auto currentAnimation = *it;
		int offset = currentAnimation->get_offset();
		int start_time = currentAnimation->get_start_time();
		
		start_time = currentAnimation->get_start_time() - offset;
		
		int end_time = currentAnimation->get_end_time();
		
		int duration = currentAnimation->get_duration();
		int wrapped_time = animationComponent->get_current_time();
		
		if(animationStack.size() == 1){
			
			// Animation
			auto bone = currentAnimation->get_wrapped_animation()->find_bone(node_name);
			if (bone != nullptr && bone->get_time_set().size() > 0)
			{
				auto local = bone->get_local_transform(wrapped_time, 1);
				
				if (mIsRootMotion)
				{
					local = glm::mat4(glm::mat3(local));
				}
				
				// Blend with the previous transformation per bone
				
				blendedLocal = local;
				
				modified = true;
			}
		} else {
			
			float blendFactor = 0.0f;
			
			// Check if there's an overlap between animations.
			if (previousEndTime > start_time) {
				// Calculate the overlap duration.
				float overlapDuration = std::min(end_time, previousEndTime) - start_time;
				float halfOverlap = overlapDuration / 2.0f;
				
				// Determine the current position within the overlap.
				float timeIntoOverlap = current_time_ - start_time;
				
				// Adjust blend factor based on position within the overlap.
				if (timeIntoOverlap <= halfOverlap) {
					// First half of the overlap: blend factor increases from 0 to 1.
					blendFactor = glm::clamp(timeIntoOverlap / halfOverlap, 0.0f, 1.0f);
				} else {
					// Second half of the overlap: blend factor decreases from 1 to 0.
					blendFactor = glm::clamp((overlapDuration - timeIntoOverlap) / halfOverlap, 0.0f, 1.0f);
				}
			} else {
				// No overlap or current animation is completely after the previous: use a default blend factor.
				// You might decide to set blendFactor to 1.0f or another value depending on your needs.
				blendFactor = 1.0f;
			}
			
			// Animation
			auto bone = currentAnimation->get_wrapped_animation()->find_bone(node_name);
			if (bone != nullptr && bone->get_time_set().size() > 0)
			{
				auto local = bone->get_local_transform(wrapped_time, 1);
				if (entity->get_mutable_parent()->get_component<ArmatureComponent>() == nullptr && mIsRootMotion)
				{
					local = glm::mat4(glm::mat3(local));
				}
				
				for (int i = 0; i < 4; ++i)
				{
					for (int j = 0; j < 4; ++j)
					{
						blendedLocal[i][j] = glm::mix(blendedLocal[i][j], local[i][j], blendFactor);
					}
				}
				
				modified = true;
			}
		}
		
		// Calculate the intersection time with the next animation
		previousIntersectionTime = (it + 1 != animationStack.end()) ? std::min(end_time, (*(it + 1))->get_start_time()) : end_time;
		
		previousDuration = duration;
		
		previousStartTime = start_time;
		previousEndTime = end_time;
	}
	
	if(modified){
		entity->set_local(blendedLocal);
	}
	
	global_transformation *= entity->get_local();
	
	auto &children = entity->get_mutable_children();
	size_t size = children.size();
	for (size_t i = 0; i < size; i++)
	{
		calculate_mesh_transform(children[i], animationComponent, animationStack, global_transformation);
	}
}

const float Animator::get_current_frame_time() const
{
	return current_time_;
}
const float Animator::get_start_time() const
{
	return start_time_;
}
const float Animator::get_end_time() const
{
	return end_time_;
}
const float Animator::get_direction() const
{
	return direction_;
}
const bool Animator::get_is_stop() const
{
	return is_stop_;
}
void Animator::set_current_time(float current_time)
{
	current_time_ = current_time;
}
void Animator::set_start_time(float time)
{
	start_time_ = time;
}
void Animator::set_end_time(float time)
{
	end_time_ = time;
}
void Animator::set_sequencer_end_time(float time)
{
	sequencer_end_time_ = time;
}
void Animator::set_mode(AnimatorMode mode){
	mode_ = mode;
	
	final_bone_matrices_.clear();
	final_bone_matrices_.reserve(MAX_BONE_NUM);
	for (unsigned int i = 0U; i < MAX_BONE_NUM; i++)
		final_bone_matrices_.push_back(glm::mat4(1.0f));
	
}
void Animator::set_direction(bool is_left)
{
	direction_ = 1.0f;
	if (is_left)
	{
		direction_ = -1.0f;
	}
}
void Animator::set_is_stop(bool is_stop)
{
	is_stop_ = is_stop;
}

}
