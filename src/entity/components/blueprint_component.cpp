#include "blueprint_component.h"

namespace anim
{

void BlueprintComponent::init(Scene& scene)
{
	blueprintProcessor = std::make_unique<BluePrint::NodeProcessor>(scene);
}

void BlueprintComponent::update(std::shared_ptr<Entity>)
{
	blueprintProcessor->Update();
}
}
