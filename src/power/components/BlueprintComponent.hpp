#pragma once

#include <memory>

namespace blueprint {
	class NodeProcessor;
}

class BlueprintComponent {
public:
	BlueprintComponent(std::unique_ptr<blueprint::NodeProcessor> nodeProcessor);
	
	blueprint::NodeProcessor& node_processor();
	
	void update();
	
private:
	std::unique_ptr<blueprint::NodeProcessor> mNodeProcessor;
};
