#pragma once

#include "graphics/drawing/Drawable.hpp"
#include <vector>
#include <functional>

class CameraManager;
class GizmoMesh;

class GizmoComponent : public Drawable {
public:
    GizmoComponent();

    void draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view, const nanogui::Matrix4f& projection) override;

private:
    std::vector<std::reference_wrapper<GizmoMesh>> mGizmos;
};
