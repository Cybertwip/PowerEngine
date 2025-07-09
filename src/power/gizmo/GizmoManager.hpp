#ifndef GIZMOMANAGER_HPP
#define GIZMOMANAGER_HPP

#include <nanogui/widget.h>
#include <nanogui/button.h>
#include <glm/glm.hpp>
#include <optional>
#include <functional>
#include <memory>

// Forward declarations for Metal types (use void* to keep header C++ compatible)
#ifdef __OBJC__
@protocol MTLDevice, MTLRenderCommandEncoder, MTLRenderPipelineState, MTLDepthStencilState;
#else
typedef void* id;
#endif

// Forward declarations for C++ classes
class Actor;
class ActorManager;

enum class GizmoMode {
	None,
	Translation,
	Rotation,
	Scale
};

enum class GizmoAxis {
	None,
	X,
	Y,
	Z
};

class GizmoManager {
public:
	// Constructor now takes a Metal device and pixel format
	GizmoManager(nanogui::Widget& parent, id<MTLDevice> device, unsigned long colorPixelFormat, ActorManager& actorManager);
	~GizmoManager();
	
	// Public interface remains the same
	void select(std::optional<std::reference_wrapper<Actor>> actor);
	void set_mode(GizmoMode mode);
	GizmoMode get_mode() const { return mCurrentMode; }
	void select_axis(GizmoAxis gizmoId);
	void transform(float dx, float dy);
	
	// Draw method now takes a Metal command encoder
	void draw(id<MTLRenderCommandEncoder> encoder, const glm::mat4& view, const glm::mat4& projection);
	
private:
	// UI elements
	std::shared_ptr<nanogui::Button> mTranslationButton;
	std::shared_ptr<nanogui::Button> mRotationButton;
	std::shared_ptr<nanogui::Button> mScaleButton;
	
	// Core references
	ActorManager& mActorManager;
	id<MTLDevice> mDevice; // Store the device
	
	// State
	GizmoMode mCurrentMode = GizmoMode::None;
	GizmoAxis mActiveAxis = GizmoAxis::None;
	std::optional<std::reference_wrapper<Actor>> mActiveActor;
	
	// Metal-specific resources (using void* for header compatibility)
	void* mPipelineState = nullptr;
	void* mDepthDisabledState = nullptr;
	
	struct GizmoMesh {
		void* vbo = nullptr; // Will be an id<MTLBuffer>
		int vertex_count = 0;
		unsigned int primitiveType; // MTLPrimitiveType
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
