#pragma once

#include "graphics/drawing/Batch.hpp"

#include <nanogui/vector.h>

#include <memory>
#include <functional>


namespace nanogui {
class Texture;
}

class Mesh;

class IMeshBatch : public Batch {
	
public:
	virtual ~IMeshBatch() = default;
	
	virtual void add_mesh(std::reference_wrapper<Mesh> mesh) = 0;
	virtual void clear() = 0;
	virtual void remove(std::reference_wrapper<Mesh> meshRef) = 0;
	
protected:
	virtual void append(std::reference_wrapper<Mesh> meshRef) = 0;
};
