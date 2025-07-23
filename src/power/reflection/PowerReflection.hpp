#pragma once

#include <string>
#include <vector>
#include <map>
#include <string_view>

// Modern, header-only reflection library for C++17
#include <refl.hpp>
#include <refl/runtime_type_info.hpp>

namespace power::reflection {

// Forward declarations
class PowerType;
class ReflectionRegistry;

// Holds information about a reflected class property (member variable).
struct PropertyInfo {
    std::string name;
    refl::runtime::type_info type;
};

// Holds information about a single parameter of a reflected method.
struct ParameterInfo {
    std::string name; // Note: refl-cpp doesn't capture param names; a placeholder (e.g., "p0") is generated.
    refl::runtime::type_info type;
    unsigned int index;
};

// Holds information about a reflected class method.
struct MethodInfo {
    std::string name;
    refl::runtime::type_info return_type;
    std::vector<ParameterInfo> parameters;
};

// A user-friendly wrapper around a refl::runtime::type_info.
class PowerType {
private:
    refl::runtime::type_info m_type;
    PowerType(refl::runtime::type_info t); // Private constructor

public:
    static PowerType get_by_name(const std::string& name);

    bool is_valid() const;
    std::string_view get_name() const;
    std::vector<PropertyInfo> get_properties() const;
    std::vector<MethodInfo> get_methods() const;
    const refl::runtime::type_info& get_underlying_type() const;

    friend class ReflectionRegistry;
};

// A registry to hold all statically discovered types.
class ReflectionRegistry {
public:
    static std::vector<PowerType> get_all_types();
    static PowerType get_type_by_name(const std::string& name);

    // Template for registering a type. Must be defined in the header.
    template<typename T>
    static void register_type() {
        // Convert compile-time descriptor to runtime info
        const auto rti = refl::runtime::type_info::create(refl::reflect<T>());
        auto& name_map = get_name_map();
        auto name_str = std::string(rti.name());
        
        // Avoid duplicate registrations
        if (name_map.find(name_str) == name_map.end()) {
            name_map[name_str] = rti;
            get_type_vector().push_back(rti);
        }
    }

private:
    // Map for quick, name-based lookups
    static std::map<std::string, refl::runtime::type_info>& get_name_map();
    // Vector to store all registered types
    static std::vector<refl::runtime::type_info>& get_type_vector();
};

// Helper to register a type at program startup.
// Note: For this to work, type T must be reflectable using the C++17 attribute,
// for example: struct [[refl::reflect]] MyClass { ... };
template<typename T>
struct AutoRegistrator {
    AutoRegistrator() {
        ReflectionRegistry::register_type<T>();
    }
};

} // namespace power::reflection