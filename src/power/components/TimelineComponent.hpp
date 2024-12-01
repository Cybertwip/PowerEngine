#pragma once

#include "AnimationComponent.hpp"
#include "TransformAnimationComponent.hpp"
#include "SkinnedAnimationComponent.hpp"
#include "actors/Actor.hpp"
#include "animation/AnimationTimeProvider.hpp"

#include <functional>
#include <memory>
#include <vector>


class TimelineComponent : public AnimationComponent {
public:
	TimelineComponent(std::vector<std::reference_wrapper<AnimationComponent>>&& components, AnimationTimeProvider& animationTimeProvider) : mAnimationTimeProvider(animationTimeProvider) {
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
	
	void SyncWithProvider() override {
		for (auto& component : mComponents) {
			component.get().SyncWithProvider();
		}
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

class SimpleTakeComponent : public AnimationComponent {
public:
	SimpleTakeComponent(std::reference_wrapper<Actor> actor, AnimationTimeProvider& timeProvider) : mActor(actor.get()), mTimeProvider(timeProvider) {
		add_timeline();
		mTimelineIndex = 0;
	}
	
	~SimpleTakeComponent() = default;
	
	virtual void add_timeline() {
		// Add TransformComponent and AnimationComponent
		auto& transform = mActor.get_component<TransformComponent>();
		
		auto& transformAnimationComponent = mAnimationComponents.emplace_back(std::make_shared<TransformAnimationComponent>(transform, mTimeProvider));
		
		std::vector<std::reference_wrapper<AnimationComponent>> components = {
			*transformAnimationComponent,
		};
		mTimelineComponents.push_back(std::make_shared<TimelineComponent>(std::move(components), mTimeProvider));
	}
	
	void set_active_timeline(unsigned int index) {
		mTimelineIndex = index;
	}
	
	void remove_timeline(size_t index) {
		if (index < mTimelineComponents.size()) {  // Check if index is within bounds
			mTimelineComponents.erase(mTimelineComponents.begin() + index);
		}
	}

	
	size_t get_num_timelines() {
		return mTimelineComponents.size();
	}
	
	TimelineComponent& get_active_timeline() {
		return *mTimelineComponents[mTimelineIndex];
	}
	
	
	void TriggerRegistration() override {
		auto& timeline = get_active_timeline();
		
		timeline.TriggerRegistration();
	}
	
	void AddKeyframe() override {
		auto& timeline = get_active_timeline();
		
		timeline.AddKeyframe();
	}
	
	void UpdateKeyframe() override {
		auto& timeline = get_active_timeline();
		
		timeline.UpdateKeyframe();
	}
	
	void RemoveKeyframe() override {
		auto& timeline = get_active_timeline();
		
		timeline.RemoveKeyframe();
	}
	
	void Freeze() override {
		auto& timeline = get_active_timeline();
		
		timeline.Freeze();
	}
	
	void Evaluate() override {
		auto& timeline = get_active_timeline();
		
		timeline.Evaluate();
	}
	
	void Unfreeze() override {
		auto& timeline = get_active_timeline();
		
		timeline.Unfreeze();
	}
	
	bool IsSyncWithProvider() override {
		auto& timeline = get_active_timeline();
		
		return timeline.IsSyncWithProvider();
	}
	
	void SyncWithProvider() override {
		auto& timeline = get_active_timeline();
		
		timeline.SyncWithProvider();
	}
	
	bool KeyframeExists() override {
		auto& timeline = get_active_timeline();
		
		return timeline.KeyframeExists();
	}
	
	float GetPreviousKeyframeTime() override {
		auto& timeline = get_active_timeline();
		
		return timeline.GetPreviousKeyframeTime();
	}
	
	float GetNextKeyframeTime() override {
		auto& timeline = get_active_timeline();
		
		return timeline.GetNextKeyframeTime();
	}

protected:
	Actor& mActor;
	AnimationTimeProvider& mTimeProvider;
	
	std::vector<std::shared_ptr<AnimationComponent>> mAnimationComponents;

	std::vector<std::shared_ptr<TimelineComponent>> mTimelineComponents;

private:
	unsigned int mTimelineIndex;
};

class SkinnedTakeComponent : public SimpleTakeComponent {
public:
	SkinnedTakeComponent(std::reference_wrapper<Actor> actor, AnimationTimeProvider& timeProvider) : SimpleTakeComponent(actor, timeProvider) {
	}
	
	~SkinnedTakeComponent() = default;
	
	void add_timeline() override {
		// Add TransformComponent and AnimationComponent
		auto& transform = mActor.get_component<TransformComponent>();
		
		auto& transformAnimationComponent = mAnimationComponents.emplace_back(std::make_unique<TransformAnimationComponent>(transform, mTimeProvider));
		
		
		// Add PlaybackComponent
		auto& playbackComponent = mActor.get_component<PlaybackComponent>();
		
		auto& skeletonComponent = mActor.get_component<SkeletonComponent>();
		
		auto& skinnedAnimationComponent = mAnimationComponents.emplace_back(std::make_unique<SkinnedAnimationComponent>(playbackComponent, skeletonComponent, mTimeProvider));
		

		std::vector<std::reference_wrapper<AnimationComponent>> components = {
			*transformAnimationComponent,
			*skinnedAnimationComponent
		};
		mTimelineComponents.push_back(std::make_shared<TimelineComponent>(std::move(components), mTimeProvider));
	}
};

class TakeComponent : AnimationComponent {
public:
	TakeComponent(std::unique_ptr<SimpleTakeComponent> takeComponent) : mTakeComponent(std::move(takeComponent)) {
	}
	
	void add_timeline() {
		mTakeComponent->add_timeline();
	}
	
	void remove_timeline(size_t index){
		mTakeComponent->remove_timeline(index);
	}
	
	size_t get_num_timelines() {
		return mTakeComponent->get_num_timelines();
	}
	
	void set_active_timeline(unsigned int index) {
		mTakeComponent->set_active_timeline(index);
	}
	
	TimelineComponent& get_active_timeline() {
		return mTakeComponent->get_active_timeline();
	}
	
	
	void TriggerRegistration() override {
		mTakeComponent->TriggerRegistration();
	}
	
	void AddKeyframe() override {
		mTakeComponent->AddKeyframe();
	}
	
	void UpdateKeyframe() override {
		mTakeComponent->UpdateKeyframe();
	}
	
	void RemoveKeyframe() override {
		mTakeComponent->RemoveKeyframe();
	}
	
	void Freeze() override {
		mTakeComponent->Freeze();
	}
	
	void Evaluate() override {
		mTakeComponent->Evaluate();
	}
	
	void Unfreeze() override {
		mTakeComponent->Unfreeze();
	}
	
	bool IsSyncWithProvider() override {
		return mTakeComponent->IsSyncWithProvider();
	}
	
	void SyncWithProvider() override {
		mTakeComponent->SyncWithProvider();
	}
	
	bool KeyframeExists() override {
		return mTakeComponent->KeyframeExists();
	}
	
	float GetPreviousKeyframeTime() override {
		return mTakeComponent->GetPreviousKeyframeTime();
	}
	
	float GetNextKeyframeTime() override {
		return mTakeComponent->GetNextKeyframeTime();
	}
	
private:
	std::unique_ptr<SimpleTakeComponent> mTakeComponent;
};
