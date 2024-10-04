#pragma once

#include "AnimationComponent.hpp"
#include "animation/AnimationTimeProvider.hpp"

#include <functional>
#include <vector>

class TimelineComponent : public AnimationComponent {
public:
	TimelineComponent(const std::vector<std::reference_wrapper<AnimationComponent>> components, AnimationTimeProvider& animationTimeProvider) : mAnimationTimeProvider(animationTimeProvider) {
		mComponents = std::move(components);
	}
	
	~TimelineComponent() = default;
	
	void TriggerRegistration() override {
		for (auto& component : mComponents) {
			component.get().TriggerRegistration();
		}
	}
	
	void AddKeyframe() override {
		for (auto& component : mComponents) {
			component.get().AddKeyframe();
		}
	}
	
	void UpdateKeyframe() override {
		for (auto& component : mComponents) {
			component.get().UpdateKeyframe();
		}
	}
	
	void RemoveKeyframe() override {
		for (auto& component : mComponents) {
			component.get().RemoveKeyframe();
		}
	}
	
	void Freeze() override {
		for (auto& component : mComponents) {
			component.get().Freeze();
		}
	}
	
	void Evaluate() override {
		for (auto& component : mComponents) {
			component.get().Evaluate();
		}
	}
	
	void Unfreeze() override {
		for (auto& component : mComponents) {
			component.get().Unfreeze();
		}
	}
	
	bool IsSyncWithProvider() override {
		for (auto& component : mComponents) {
			if (!component.get().IsSyncWithProvider()) {
				return false;
			}
		}
				
		return true;
	}
	
	bool KeyframeExists() override {
		for (auto& component : mComponents) {
			if (component.get().KeyframeExists()) {
				return true;
			}
		}
		
		return false;
	}
	
	float GetPreviousKeyframeTime() override {
		float currentTimeFloat = static_cast<float>(mAnimationTimeProvider.GetTime());
		
		float latestPrevTime = -std::numeric_limits<float>::infinity();
		bool hasPrevKeyframe = false;
		
		for (auto& component : mComponents) {
			float prevTime = component.get().GetPreviousKeyframeTime();
			if (prevTime != -1.0f) { // Assuming -1 indicates no keyframe
				hasPrevKeyframe = true;
				if (prevTime > latestPrevTime) {
					latestPrevTime = prevTime;
				}
			}
		}
		
		return hasPrevKeyframe ? latestPrevTime : -1.0f;
	}


	float GetNextKeyframeTime() override {
		float currentTimeFloat = static_cast<float>(mAnimationTimeProvider.GetTime());
		
		float earliestNextTime = std::numeric_limits<float>::infinity();
		bool hasNextKeyframe = false;
		
		for (auto& component : mComponents) {
			float nextTime = component.get().GetNextKeyframeTime();
			if (nextTime != -1.0f) { // Assuming -1 indicates no keyframe
				hasNextKeyframe = true;
				if (nextTime < earliestNextTime) {
					earliestNextTime = nextTime;
				}
			}
		}
		
		return hasNextKeyframe ? earliestNextTime : -1.0f;
	}

private:
	std::vector<std::reference_wrapper<AnimationComponent>> mComponents;
	
	AnimationTimeProvider& mAnimationTimeProvider;
};
