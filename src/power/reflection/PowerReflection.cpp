#include "PowerReflection.hpp"
#include <algorithm> // For std::sort
#include <string>    // For std::to_string

namespace power::reflection {

// --- PowerType Implementation ---

PowerType PowerType::get_by_name(const std::string& name) {
	return ReflectionRegistry::get_type_by_name(name);
}

std::vector<PropertyInfo> PowerType::get_properties() const {
	if (!m_is_valid) return {};
	// Simply invoke the stored function to generate the info.
	return m_properties_getter();
}

std::vector<MethodInfo> PowerType::get_methods() const {
	if (!m_is_valid) return {};
	// Simply invoke the stored function to generate the info.
	return m_methods_getter();
}

const PowerType::NodeCreatorFunc& PowerType::get_node_creator() const {
	// A non-const reference can't be returned if the object is const.
	// We return a const reference to the stored function.
	if (!m_is_valid) {
		static const NodeCreatorFunc invalid_creator = nullptr;
		return invalid_creator;
	}
	return m_node_creator;
}

bool PowerType::has_node_creator() const {
	// A std::function is convertible to bool. It's true if it holds a target.
	return m_is_valid && static_cast<bool>(m_node_creator);
}


// --- ReflectionRegistry Implementation ---

std::map<std::string, PowerType>& ReflectionRegistry::get_registry() {
	static std::map<std::string, PowerType> registry;
	return registry;
}

PowerType ReflectionRegistry::get_type_by_name(const std::string& name) {
	const auto& map = get_registry();
	if (auto it = map.find(name); it != map.end()) {
		return it->second;
	}
	return PowerType{}; // Return a default (invalid) PowerType
}

std::vector<PowerType> ReflectionRegistry::get_all_types() {
	auto& reg = get_registry();
	std::vector<PowerType> types;
	types.reserve(reg.size());
	
	for (const auto& pair : reg) {
		types.push_back(pair.second);
	}
	
	// Sort by name for consistent ordering.
	std::sort(types.begin(), types.end(), [](const PowerType& a, const PowerType& b) {
		return a.get_name() < b.get_name();
	});
	
	return types;
}

} // namespace power::reflection
