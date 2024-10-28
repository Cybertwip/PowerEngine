#pragma once

class ISkeleton;

class SkeletonComponent {
public:
	SkeletonComponent(ISkeleton& skeleton) : mSkeleton(skeleton) {
		
	}
	
	ISkeleton& get_skeleton() const {
		return mSkeleton;
	}
	
	private:
	ISkeleton& mSkeleton;
};

