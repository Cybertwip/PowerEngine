#pragma once

#include <string>
#include <vector>
#include <rttr/type.h>

namespace power::reflection {

// Forward declarations
class PowerType;
class ReflectionRegistry;

// Holds information about a reflected class property (member variable).
struct PropertyInfo {
    std::string name;
    rttr::type type;
};

// Holds information about a single parameter of a reflected method.
struct ParameterInfo {
    std::string name;
    rttr::type type;
    unsigned int index;
};

// Holds information about a reflected class method.
struct MethodInfo {
    std::string name;
    rttr::type return_type;
    std::vector<ParameterInfo> parameters;
};

// A user-friendly wrapper around an rttr::type for the Power Reflection system.
class PowerType {
private:
    rttr::type m_type;
    PowerType(rttr::type t); // Private constructor

public:
    static PowerType get_by_name(const std::string& name);

    bool is_valid() const;
    std::string get_name() const;
    std::vector<PropertyInfo> get_properties() const;
    std::vector<MethodInfo> get_methods() const;
    rttr::type get_underlying_type() const;

    // Allow ReflectionRegistry to access the private constructor
    friend class ReflectionRegistry;
};

// A registry to hold all statically discovered types.
class ReflectionRegistry {
public:
    static std::vector<PowerType> get_all_types();
    static void register_type(rttr::type type);

private:
    static std::vector<rttr::type>& get_registry();
};

// Helper struct used by generated code to register a type at program startup.
// This must remain in the header file to be accessible by the generated code.
template<typename T>
struct AutoRegistrator {
    AutoRegistrator() {
        ReflectionRegistry::register_type(rttr::type::get<T>());
    }
};

} // namespace power::reflection
