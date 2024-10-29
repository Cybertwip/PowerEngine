#pragma once


class IBone;

class ISkeleton {
public:
	ISkeleton() {
		
	}
	
	virtual ~ISkeleton() = default;
	
	// Get the number of bones
	virtual int num_bones() const = 0;
	// Get mutable bone by index
	virtual IBone& get_bone(int index) = 0;

	virtual int get_bone_index(IBone& bone) = 0;

	virtual void remap_bone(int index, int new_parent_index, bool cleanup = false) = 0;
	
	virtual void trim_bone(int index) = 0;
};
