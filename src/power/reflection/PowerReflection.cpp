#include "PowerReflection.hpp"
#include <algorithm> // For std::sort
#include <string>    // For std::to_string

namespace power::reflection {

// --- PowerType Implementation ---

PowerType::PowerType(refl::runtime::type_info t) : m_type(t) {}

PowerType PowerType::get_by_name(const std::string& name) {
    return ReflectionRegistry::get_type_by_name(name);
}

bool PowerType::is_valid() const {
    return m_type.is_valid();
}

std::string_view PowerType::get_name() const {
    return is_valid() ? m_type.name() : "Invalid Type";
}

std::vector<PropertyInfo> PowerType::get_properties() const {
    if (!is_valid()) return {};
    
    std::vector<PropertyInfo> result;
    for (const auto& prop : m_type.get_public_members()) {
        result.emplace_back(PropertyInfo{std::string(prop.name), prop.type});
    }
    return result;
}

std::vector<MethodInfo> PowerType::get_methods() const {
    if (!is_valid()) return {};
    
    std::vector<MethodInfo> result;
    for (const auto& method : m_type.get_public_functions()) {
        std::vector<ParameterInfo> params;
        for (size_t i = 0; i < method.parameters.size(); ++i) {
            params.emplace_back(ParameterInfo{
                "p" + std::to_string(i), // Generate placeholder name
                method.parameters[i],
                static_cast<unsigned int>(i)
            });
        }
        
        result.emplace_back(MethodInfo{
            std::string(method.name),
            method.return_type,
            std::move(params)
        });
    }
    return result;
}

const refl::runtime::type_info& PowerType::get_underlying_type() const {
    return m_type;
}


// --- ReflectionRegistry Implementation ---

std::map<std::string, refl::runtime::type_info>& ReflectionRegistry::get_name_map() {
    static std::map<std::string, refl::runtime::type_info> name_map;
    return name_map;
}

std::vector<refl::runtime::type_info>& ReflectionRegistry::get_type_vector() {
    static std::vector<refl::runtime::type_info> type_vector;
    return type_vector;
}

PowerType ReflectionRegistry::get_type_by_name(const std::string& name) {
    const auto& map = get_name_map();
    if (auto it = map.find(name); it != map.end()) {
        return PowerType(it->second);
    }
    return PowerType(refl::runtime::type_info{}); // Return an invalid PowerType
}

std::vector<PowerType> ReflectionRegistry::get_all_types() {
    auto& reg = get_type_vector();
    // Sort by name for consistent ordering
    std::sort(reg.begin(), reg.end(), [](const auto& a, const auto& b) {
        return a.name() < b.name();
    });
    
    std::vector<PowerType> types;
    types.reserve(reg.size());
    
    for (const auto& rti : reg) {
        types.push_back(PowerType(rti));
    }
    return types;
}

} // namespace power::reflection