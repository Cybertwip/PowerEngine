#pragma once

#include "component.h"
#include <cassert>

namespace anim
{
    class ModelSerializationComponent : public ComponentBase<ModelSerializationComponent>
    {
	public:
		void set_model_path(const std::string& path){
			assert(path.size() > 0 && "Must be a valid path");
			_path = path;
		}
		
		const std::string& get_model_path() const {
			assert(_path.size() > 0 && "Must be a valid path");
			return _path;
		}
		
	private:
		
		std::shared_ptr<Component> clone() override {
			assert(false && "Not implemented");
		}

		std::string _path;
    };
}
