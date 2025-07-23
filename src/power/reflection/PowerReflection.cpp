// src/reflection.cpp
#include "reflection.h"
#include <algorithm> // For std::sort and std::find
#include <rttr/core/property.h>
#include <rttr/core/method.h>
#include <rttr/core/parameter_info.h>

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
        result.push_back({prop.get_name().to_string(), prop.get_type()});
    }
    return result;
}

std::vector<MethodInfo> PowerType::get_methods() const {
    if (!is_valid()) return {};

    std::vector<MethodInfo> result;
    for (const auto& method : m_type.get_methods()) {
        MethodInfo info;
        info.name = method.get_name().to_string();
        info.return_type = method.get_return_type();
        
        for (const auto& param : method.get_parameter_infos()) {
            info.parameters.push_back({
                param.get_name().to_string(),
                param.get_type(),
                param.get_index()
            });
        }
        result.push_back(info);
    }
    return result;
}

rttr::type PowerType::get_underlying_type() const {
    return m_type;
}


// --- ReflectionRegistry Implementation ---

std::vector<rttr::type>& ReflectionRegistry::get_registry() {
    // Meyers' Singleton for safe static initialization
    static std::vector<rttr::type> registry;
    return registry;
}

void ReflectionRegistry::register_type(rttr::type type) {
    if (type.is_valid()) {
        auto& reg = get_registry();
        // Avoid duplicates
        if (std::find(reg.begin(), reg.end(), type) == reg.end()) {
            reg.push_back(type);
        }
    }
}

std::vector<PowerType> ReflectionRegistry::get_all_types() {
    auto& reg = get_registry();
    // Sort registry alphabetically for consistent ordering
    std::sort(reg.begin(), reg.end(), [](const rttr::type& a, const rttr::type& b) {
        return a.get_name() < b.get_name();
    });
    
    std::vector<PowerType> types;
    for (const auto& type : reg) {
        types.emplace_back(type); // Uses the private PowerType constructor
    }
    return types;
}

} // namespace power::reflection
