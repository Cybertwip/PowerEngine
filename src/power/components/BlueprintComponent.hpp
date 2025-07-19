#pragma once

#include <memory>

class NodeProcessor;

class BlueprintComponent {
public:
	BlueprintComponent(std::unique_ptr<NodeProcessor> nodeProcessor);
	
	NodeProcessor& node_processor() const;

	void update();
	
private:
	std::unique_ptr<NodeProcessor> mNodeProcessor;
};
