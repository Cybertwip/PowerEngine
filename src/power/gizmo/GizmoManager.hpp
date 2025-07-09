#ifndef GIZMOMANAGER_HPP
#define GIZMOMANAGER_HPP

#include <nanogui/widget.h>
#include <nanogui/button.h>
#include <glm/glm.hpp>
#include <optional>
#include <functional>
#include <memory>

// Forward declarations for C++ classes
class Actor;
class ActorManager;

// Enum for Gizmo mode (Translate, Rotate, Scale)
enum class GizmoMode {
	None,
	Translation,
	Rotation,
	Scale
};

// Enum for the selected Gizmo axis
enum class GizmoAxis {
	None,
	X,
	Y,
	Z
};

class GizmoManager {
public:
	// Constructor and Draw now take void* for Metal objects
	// to keep this header pure C++. The implementation will
	// cast these to the appropriate Metal types.
	GizmoManager(nanogui::Widget& parent, void* device, unsigned long colorPixelFormat, ActorManager& actorManager);
	~GizmoManager();
	
	void select(std::optional<std::reference_wrapper<Actor>> actor);
	void set_mode(GizmoMode mode);
	GizmoMode get_mode() const { return mCurrentMode; }
	void select_axis(GizmoAxis gizmoId);
	void transform(float dx, float dy);
	
	// The `encoder` parameter should be an `id<MTLRenderCommandEncoder>`
	void draw(void* encoder, const glm::mat4& view, const glm::mat4& projection);
	
private:
	// UI elements
	std::shared_ptr<nanogui::Button> mTranslationButton;
	std::shared_ptr<nanogui::Button> mRotationButton;
	std::shared_ptr<nanogui::Button> mScaleButton;
	
	// Core references
	ActorManager& mActorManager;
	void* mDevice = nullptr; // Opaque pointer to an id<MTLDevice>
	
	// State
	GizmoMode mCurrentMode = GizmoMode::None;
	GizmoAxis mActiveAxis = GizmoAxis::None;
	std::optional<std::reference_wrapper<Actor>> mActiveActor;
	
	// Metal-specific resources (as opaque pointers)
	void* mPipelineState = nullptr;      // Opaque pointer to an id<MTLRenderPipelineState>
	void* mDepthDisabledState = nullptr; // Opaque pointer to an id<MTLDepthStencilState>
	
	struct GizmoMesh {
		void* vbo = nullptr; // Opaque pointer to an id<MTLBuffer>
		int vertex_count = 0;
		unsigned int primitiveType; // Corresponds to MTLPrimitiveType enum
	};
	
	GizmoMesh mTranslationMeshes[3];
	GizmoMesh mRotationMeshes[3];
	GizmoMesh mScaleMeshes[3];
	
	// Initialization helpers
	void create_procedural_gizmos();
	void create_translation_gizmo();
	void create_rotation_gizmo();
	void create_scale_gizmo();
	
	// Transformation logic
	void translate(float dx, float dy);
	void rotate(float dx, float dy);
	void scale(float dx, float dy);
};

#endif // GIZMOMANAGER_HPP
