#pragma once

#include <cmath>

class AnimationTimeProvider {
public:
	AnimationTimeProvider(float duration) : mTime(0.0f), mDuration(duration) {
		
	}
	virtual ~AnimationTimeProvider() = default;
	
	virtual void Update(float time = 0.0f) {
		mTime = time;
		mTime = std::fmod(mTime, mDuration);
	}
	
	float GetTime() {
		return mTime;
	}
	
private:
	float mTime;
	float mDuration;
};
