#include "BlueprintComponent.hpp"

#include "execution/BlueprintManager.hpp"

BlueprintComponent::BlueprintComponent(std::unique_ptr<NodeProcessor> nodeProcessor)
: mNodeProcessor(std::move(nodeProcessor)) {
	
}

NodeProcessor& BlueprintComponent::node_processor() {
	return *mNodeProcessor;
}

void BlueprintComponent::update() {
	mNodeProcessor->evaluate();
}
