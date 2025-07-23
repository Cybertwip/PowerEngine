#include "PowerReflection.hpp"
#include <algorithm> // For std::sort and std::find
#include <rttr/property.h>
#include <rttr/method.h>
#include <rttr/parameter_info.h>
#include <rttr/argument.h>

namespace power::reflection {

// --- PowerType Implementation ---

PowerType::PowerType(rttr::type t) : m_type(t) {}

PowerType PowerType::get_by_name(const std::string& name) {
	return PowerType(rttr::type::get_by_name(name));
}

bool PowerType::is_valid() const {
	return m_type.is_valid();
}

std::string PowerType::get_name() const {
	return is_valid() ? m_type.get_name().to_string() : "Invalid Type";
}

std::vector<PropertyInfo> PowerType::get_properties() const {
	if (!is_valid()) return {};
	
	std::vector<PropertyInfo> result;
	for (const auto& prop : m_type.get_properties()) {
		result.emplace_back(PropertyInfo{prop.get_name().to_string(), prop.get_type()});
	}
	return result;
}

std::vector<MethodInfo> PowerType::get_methods() const {
	if (!is_valid()) return {};
	
	std::vector<MethodInfo> result;
	for (const auto& method : m_type.get_methods()) {
		std::vector<ParameterInfo> params;
		for (const auto& param : method.get_parameter_infos()) {
			params.emplace_back(ParameterInfo{
				param.get_name().to_string(),
				param.get_type(),
				param.get_index()
			});
		}
		
		result.emplace_back(MethodInfo{
			method.get_name().to_string(),
			method.get_return_type(),
			std::move(params)
		});
	}
	return result;
}

rttr::type PowerType::get_underlying_type() const {
	return m_type;
}


// --- ReflectionRegistry Implementation ---

std::vector<rttr::type>& ReflectionRegistry::get_registry() {
	static std::vector<rttr::type> registry;
	return registry;
}

void ReflectionRegistry::register_type(rttr::type type) {
	if (type.is_valid()) {
		auto& reg = get_registry();
		if (std::find(reg.begin(), reg.end(), type) == reg.end()) {
			reg.push_back(type);
		}
	}
}

std::vector<PowerType> ReflectionRegistry::get_all_types() {
	auto& reg = get_registry();
	std::sort(reg.begin(), reg.end(), [](const rttr::type& a, const rttr::type& b) {
		return a.get_name() < b.get_name();
	});
	
	std::vector<PowerType> types;
	types.reserve(reg.size()); // Optional: pre-allocate memory for efficiency
	
	for (const auto& type : reg) {
		// CHANGE: Use push_back with an explicit call to the private constructor.
		// This is allowed because we are inside a member function of the friend class.
		types.push_back(PowerType(type));
	}
	return types;
}

} // namespace power::reflection
