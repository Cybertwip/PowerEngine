#include "BlueprintComponent.hpp"

#include "components/BlueprintMetadataComponent.hpp"
#include "execution/NodeProcessor.hpp"
#include "serialization/BlueprintSerializer.hpp"


BlueprintComponent::BlueprintComponent(std::unique_ptr<NodeProcessor> nodeProcessor)
: mNodeProcessor(std::move(nodeProcessor)) {}

NodeProcessor& BlueprintComponent::node_processor() const {
    return *mNodeProcessor;
}

void BlueprintComponent::update() {
    mNodeProcessor->evaluate();
}

// MODIFIED: Function implemented
void BlueprintComponent::save_blueprint(const std::string& to_file) {
    BlueprintSerializer serializer;
    serializer.serialize(this->node_processor(), to_file);
}
