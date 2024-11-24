#include "BlueprintComponent.hpp"

#include "execution/BlueprintManager.hpp"

BlueprintComponent::BlueprintComponent(std::unique_ptr<blueprint::NodeProcessor> nodeProcessor)
: mNodeProcessor(std::move(nodeProcessor)) {
	
}

blueprint::NodeProcessor& BlueprintComponent::node_processor() {
	return *mNodeProcessor;
}

void BlueprintComponent::update() {
	mNodeProcessor->evaluate();
}
