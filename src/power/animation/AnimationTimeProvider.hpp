#pragma once

#include <cmath>

class AnimationTimeProvider {
public:
	AnimationTimeProvider(float duration) : mTime(0.0f), mDuration(duration) {
		
	}
	
	void Update() {
		mTime = std::fmod(mTime, mDuration);
	}
	
	float GetTime() {
		return mTime;
	}
	
private:
	float mTime;
	float mDuration;
};
