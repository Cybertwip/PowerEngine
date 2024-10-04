#pragma once

#include "AnimationTimeProvider.hpp"

class StackableAnimationTimeProvider : public AnimationTimeProvider {
public:
	StackableAnimationTimeProvider(AnimationTimeProvider& wrappedAnimationTimeProvider) : mSrappedAnimationTimeProvider(wrappedAnimationTimeProvider) {
		
	}
	
	~StackableAnimationTimeProvider() = default;
	
	void PushTime(float time) {
		mIsStacked = true;
	}
	
	void Update(float time = 0.0f) override {
		if (mIsStacked) {
			mStackedTime = time;
		} else {
			mWrappedAnimationTimeProvider.Update(time);
		}
	}
	
	void PopTime() {
		mIsStacked = false;
		mStackedTime = 0.0f;
	}
	
	float GetTime() {
		if (mIsStacked) {
			return mStackedTime;
		} else {
			return mWrappedAnimationTimeProvider.GetTime();
		}
	}
	
private:
	bool mIsStacked;
	float mStackedTime;
	AnimationTimeProvider& mWrappedAnimationTimeProvider;
};
