#include "SharedSelfContainedMeshCanvas.hpp"

#if defined(NANOGUI_USE_METAL)
#include "MetalHelper.hpp"
#endif

SharedSelfContainedMeshCanvas::SharedSelfContainedMeshCanvas(Widget* parent)
: SelfContainedMeshCanvas(parent) {

}

void SharedSelfContainedMeshCanvas::set_active_actor(std::shared_ptr<Actor> actor) {
	mSharedPreviewActor = actor;
	
	if (mSharedPreviewActor != nullptr) {
		SelfContainedMeshCanvas::set_active_actor(*mSharedPreviewActor);
	}
}

void SharedSelfContainedMeshCanvas::take_snapshot(std::function<void(std::vector<uint8_t>&)> onSnapshotTaken) {
	mSnapshotCallback = onSnapshotTaken;
}
