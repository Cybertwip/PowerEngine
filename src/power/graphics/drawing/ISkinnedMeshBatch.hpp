#pragma once

#include "graphics/drawing/Batch.hpp"

#include <nanogui/vector.h>

#include <memory>
#include <functional>


namespace nanogui {
class Texture;
}

class SkinnedMesh;

class ISkinnedMeshBatch : public Batch {
	
public:
	virtual ~ISkinnedMeshBatch() = default;
	
	virtual void add_mesh(std::reference_wrapper<SkinnedMesh> mesh) = 0;
	virtual void clear() = 0;
	virtual void append(std::reference_wrapper<SkinnedMesh> meshRef) = 0;
	virtual void remove(std::reference_wrapper<SkinnedMesh> meshRef) = 0;
	
};
