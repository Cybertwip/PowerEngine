#pragma once

#include <ozz/base/memory/unique_ptr.h>
#include <ozz/animation/runtime/animation.h>

#include <memory>

class SkinnedAnimationComponent {
public:
	struct SkinnedAnimationPdo {
		std::vector<ozz::reference_wrapper<ozz::animation::Animation>> mAnimationData;
	};
public:
	
	SkinnedAnimationComponent(SkinnedAnimationPdo& animationPoc) {
		
		for (auto& animation : animationPoc.mAnimationData) {
			mAnimationData.push_back(animation);
		}
		
	}
	
private:
	std::vector<std::reference_wrapper<ozz::animation::Animation>> mAnimationData;
};
