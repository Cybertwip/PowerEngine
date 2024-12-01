#pragma once

#include <cmath>

class AnimationTimeProvider {
public:
	AnimationTimeProvider(float duration) : mTime(0.0f), mDuration(duration) {
		
	}
	
	virtual ~AnimationTimeProvider() = default;
	
	virtual void Update(float time = 0.0f) {
		if (time >= mDuration) {
			mTime = mDuration;
		} else if (time < 0.0f) {
			mTime = 0;
		} else {
			mTime = std::fmod(time, mDuration);
		}
	}
	
	float GetTime() {
		return mTime;
	}
	
	void SetTime(float time) {
		mTime = time;
	}
	
private:
	float mTime;
	float mDuration;
};
