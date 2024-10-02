#pragma once

#include "SelfContainedMeshCanvas.hpp"

class SharedSelfContainedMeshCanvas : public SelfContainedMeshCanvas {
public:
	SharedSelfContainedMeshCanvas(Widget* parent);
	
	void set_active_actor(std::shared_ptr<Actor> actor);

	void take_snapshot(std::function<void(std::vector<uint8_t>&)> onSnapshotTaken);
	
	void process_events();

private:
	void draw_contents() override;
	
	std::shared_ptr<Actor> mSharedPreviewActor;
	std::function<void(std::vector<uint8_t>&)> mSnapshotCallback;
};
