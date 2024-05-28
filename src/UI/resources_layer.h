#pragma once

#include <vector>
#include <memory>
#include <string>
#include "ui_context.h"

class Scene;

namespace anim
{
class Entity;
class SharedResources;
}

namespace ui
{
// Define somewhere accessible
struct DragSourceDetails {
	bool isActive = false;
	std::string type;
	std::string payload;
	std::string displayText;
	std::string directory;
};
class TimelineLayer;
class ResourcesLayer
{
public:
	ResourcesLayer(TimelineLayer& timelineLayer) : timeline_layer_(timelineLayer){
		
	}
	
	~ResourcesLayer() = default;
	void draw(Scene *scene, UiContext &context);
	
private:
	inline void init_context(UiContext &ui_context, Scene *scene);
	Scene *scene_;
	
	anim::SharedResources *resources_;
	
	bool is_trigger_up_ = false;
	
	TimelineLayer& timeline_layer_;
	
	DragSourceDetails dragSourceDetails_;

};
}
