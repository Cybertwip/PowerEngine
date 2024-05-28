#pragma once

#include "component.h"

#include "blueprint/blueprint.h"

#include <cassert>
#include <memory>

namespace anim
{
    class BlueprintComponent : public ComponentBase<BlueprintComponent>
    {
    public:
		void init(Scene& scene);
		
		void update(std::shared_ptr<Entity> entity) override;
		
		std::shared_ptr<Component> clone() override {

			assert(false && "Not implemented");
			
			return nullptr;
		}
		
		BluePrint::NodeProcessor& get_processor() {
			return *blueprintProcessor;
		}

        
	private:
		std::unique_ptr<BluePrint::NodeProcessor> blueprintProcessor;

    };
}
