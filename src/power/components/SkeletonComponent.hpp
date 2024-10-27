#pragma once

#include <memory>

class ISkeleton;

class SkeletonComponent {
public:
	SkeletonComponent(std::unique_ptr<ISkeleton> skeleton) : mSkeleton(std::move(skeleton)) {
		
	}
	
	ISkeleton& get_skeleton() const {
		return *mSkeleton;
	}
	
	private:
	std::unique_ptr<ISkeleton> mSkeleton;
};

