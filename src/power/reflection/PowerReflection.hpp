#pragma once

#include <string>
#include <vector>
#include <map>
#include <string_view>
#include <functional> // Required for std::function
#include <type_traits> // Required for std::is_invocable_v

// Modern, header-only reflection library for C++17
#include "refl.hpp" // Ensure this path is correct for your project

namespace power::reflection {

// Forward declarations
class PowerType;
class ReflectionRegistry;

// Info structs now store type names as strings.
struct PropertyInfo {
	std::string name;
	std::string type_name;
};

struct ParameterInfo {
	std::string name; // Note: refl-cpp doesn't capture param names; a placeholder is generated.
	std::string type_name;
	unsigned int index;
};

struct MethodInfo {
	std::string name;
	std::string return_type_name;
	std::vector<ParameterInfo> parameters; // Note: will be empty due to refl-cpp limitations
};

// A type-erased, runtime-accessible wrapper for a reflected type.
class PowerType {
private:
	std::string m_name;
	bool m_is_valid = false;
	// Lambdas to generate info on demand, captured at registration time.
	std::function<std::vector<PropertyInfo>()> m_properties_getter;
	std::function<std::vector<MethodInfo>()> m_methods_getter;
	
	PowerType() = default; // For creating an invalid type
	
public:
	// Constructor used by the registry to create a valid, populated object.
	PowerType(
			  std::string_view name,
			  std::function<std::vector<PropertyInfo>()> prop_getter,
			  std::function<std::vector<MethodInfo>()> meth_getter)
	: m_name(name),
	m_is_valid(true),
	m_properties_getter(std::move(prop_getter)),
	m_methods_getter(std::move(meth_getter)) {}
	
	static PowerType get_by_name(const std::string& name);
	
	bool is_valid() const { return m_is_valid; }
	const std::string& get_name() const { return m_name; }
	
	std::vector<PropertyInfo> get_properties() const;
	std::vector<MethodInfo> get_methods() const;
	
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
		// CHANGE: Get the type name directly from the descriptor's 'name' member.
		const std::string type_name(refl::reflect<T>().name);
		
		auto& reg = get_registry();
		if (reg.count(type_name)) {
			return; // Already registered
		}
		
		// Create a lambda to generate property info at runtime.
		auto prop_getter = []() -> std::vector<PropertyInfo> {
			std::vector<PropertyInfo> props;
			refl::util::for_each(refl::reflect<T>().members, [&](auto member) {
				// CHANGE: Use 'if constexpr' to process only fields (properties).
				if constexpr (refl::descriptor::is_field(member)) {
					props.push_back({
						.name = std::string(member.name),
						// CHANGE: Get the field's type name by reflecting its type.
						.type_name = std::string(refl::reflect<typename decltype(member)::value_type>().name)
					});
				}
			});
			return props;
		};
		
		// Create a lambda to generate method info at runtime.
		auto meth_getter = []() -> std::vector<MethodInfo> {
			std::vector<MethodInfo> meths;
			// CHANGE: Iterate over all members, not a non-existent '.functions' member.
			refl::util::for_each(refl::reflect<T>().members, [&](auto func) {
				// CHANGE: Use 'if constexpr' to process only functions.
				if constexpr (refl::descriptor::is_function(func)) {
					MethodInfo mi;
					mi.name = std::string(func.name);
					
					// CHANGE: This is the modern C++ way to check for a method signature.
					// Here, we assume a simple, no-argument member function.
					if constexpr (std::is_invocable_v<decltype(func), T&>) {
						using return_type = decltype(func(std::declval<T&>()));
						mi.return_type_name = std::string(refl::reflect<return_type>().name);
					} else {
						mi.return_type_name = "(unsupported signature)";
					}
					
					// NOTE: refl-cpp does not support iterating function parameters.
					// The 'parameters' vector will remain empty.
					
					meths.push_back(std::move(mi));
				}
			});
			return meths;
		};
		
		// Create the runtime PowerType object and store it in the registry.
		reg.emplace(type_name, PowerType(type_name, std::move(prop_getter), std::move(meth_getter)));
	}
	
private:
	// The registry now stores the fully-formed PowerType objects.
	static std::map<std::string, PowerType>& get_registry();
};

// Helper to register a type at program startup.
// CHANGE: Corrected the comment to reflect how refl-cpp works.
// Note: For this to work, type T must be made reflectable using the
// REFL_AUTO() or REFL_TYPE() macros in a global scope.
template<typename T>
struct AutoRegistrator {
	AutoRegistrator() {
		ReflectionRegistry::register_type<T>();
	}
};

} // namespace power::reflection
