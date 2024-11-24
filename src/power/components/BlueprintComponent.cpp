#pragma once

#include <memory>

namespace blueprint {
	class NodeProcessor;
}

class BlueprintComponent {
public:
	BlueprintComponent(std::unique_ptr<blueprint::NodeProcessor> nodeProcessor)
	: mNodeProcessor(std::move(nodeProcessor)) {
		
	}
	
	blueprint::NodeProcessor& node_processor() {
		return *mNodeProcessor;
	}
	
private:
	std::unique_ptr<blueprint::NodeProcessor> mNodeProcessor;
};