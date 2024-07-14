#pragma once

#include "graphics/mesh.h"


class LightManager;

namespace anim{
class Mesh;
}

namespace physics {
std::unique_ptr<anim::Mesh> CreateCuboid(const glm::vec3& extents, LightManager& lightManager);
std::vector<anim::Vertex> CreateCuboidVertices(const glm::vec3& extents);
}
