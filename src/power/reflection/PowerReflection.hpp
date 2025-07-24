#pragma once

#include "serialization/UUID.hpp"

#include "refl.hpp"

#include <string>
#include <vector>
#include <map>
#include <string_view>
#include <functional>    // Required for std::function
#include <type_traits>   // Required for std::is_invocable_v
#include <memory>        // Required for std::unique_ptr
#include <any>           // Required for std::any

// Forward declarations to manage dependencies on the blueprint system
class CoreNode;
class ReflectedCoreNode;

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
public:
	// A factory function that can create a fully configured CoreNode.
	using NodeCreatorFunc = std::function<std::unique_ptr<CoreNode>(UUID)>;
	
private:
	std::string m_name;
	bool m_is_valid = false;
	// Lambdas to generate info on demand, captured at registration time.
	std::function<std::vector<PropertyInfo>()> m_properties_getter;
	std::function<std::vector<MethodInfo>()> m_methods_getter;
	// New member to hold the node factory function.
	NodeCreatorFunc m_node_creator;
	
	PowerType() = default; // For creating an invalid type
	
public:
	// Constructor used by the registry to create a valid, populated object.
	PowerType(
			  std::string_view name,
			  std::function<std::vector<PropertyInfo>()> prop_getter,
			  std::function<std::vector<MethodInfo>()> meth_getter,
			  NodeCreatorFunc node_creator) // Added node_creator parameter
	: m_name(name),
	m_is_valid(true),
	m_properties_getter(std::move(prop_getter)),
	m_methods_getter(std::move(meth_getter)),
	m_node_creator(std::move(node_creator)) {} // Initialize new member
	
	static PowerType get_by_name(const std::string& name);
	
	bool is_valid() const { return m_is_valid; }
	const std::string& get_name() const { return m_name; }
	
	std::vector<PropertyInfo> get_properties() const;
	std::vector<MethodInfo> get_methods() const;
	
	// New methods to access the node creator.
	const NodeCreatorFunc& get_node_creator() const;
	bool has_node_creator() const;
	
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
		// NOTE: For this to compile, the definition for ReflectedCoreNode must be available.
		// The include is placed here to scope the dependency to this template's instantiation.
#include "execution/nodes/ReflectedNode.hpp"
		
		const std::string type_name(refl::reflect<T>().name);
		
		auto& reg = get_registry();
		if (reg.count(type_name)) {
			return; // Already registered
		}
		
		// Create a lambda to generate property info at runtime.
		auto prop_getter = []() -> std::vector<PropertyInfo> {
			std::vector<PropertyInfo> props;
			refl::util::for_each(refl::reflect<T>().members, [&](auto member) {
				if constexpr (refl::descriptor::is_field(member)) {
					props.push_back({
						.name = std::string(member.name),
						.type_name = std::string(refl::reflect<typename decltype(member)::value_type>().name)
					});
				}
			});
			return props;
		};
		
		// Create a lambda to generate method info at runtime.
		auto meth_getter = []() -> std::vector<MethodInfo> {
			std::vector<MethodInfo> meths;
			refl::util::for_each(refl::reflect<T>().members, [&](auto func) {
				if constexpr (refl::descriptor::is_function(func)) {
					MethodInfo mi;
					mi.name = std::string(func.name);
					
					// This is a simplified check. A full implementation would be more complex.
					if constexpr (std::is_invocable_v<decltype(func), T&>) {
						using return_type = decltype(func(std::declval<T&>()));
						mi.return_type_name = std::string(refl::reflect<return_type>().name);
					} else {
						mi.return_type_name = "(unsupported signature)";
					}
					
					meths.push_back(std::move(mi));
				}
			});
			return meths;
		};
		
		// Create the Node Creator lambda.
		auto node_creator = [](UUID id) -> std::unique_ptr<CoreNode> {
			PowerType type_info = PowerType::get_by_name(std::string(refl::reflect<T>().name));
			auto instance = std::make_any<T>();
			auto node = std::make_unique<ReflectedCoreNode>(id, type_info, instance);
			
			// Populate the node's dispatchers (property getters/setters, method callers)
			refl::util::for_each(refl::reflect<T>().members, [&](auto member) {
				if constexpr (refl::descriptor::is_field(member)) {
					std::string prop_name = member.name;
					node->m_property_getters[prop_name] = [member](std::any& obj) -> std::any {
						return member(std::any_cast<T&>(obj));
					};
					node->m_property_setters[prop_name] = [member](std::any& obj, std::any val) {
						using TField = typename decltype(member)::value_type;
						if (const auto* pval = std::any_cast<TField>(&val)) {
							member(std::any_cast<T&>(obj)) = *pval;
						}
					};
				} else if constexpr (refl::descriptor::is_function(member)) {
					node->m_method_dispatchers[std::string(member.name)] =
					[f = member, node_ptr = node.get()](const std::vector<std::any>& args) {
						// A full implementation requires complex argument unpacking.
						// This simplified version only handles specific known functions.
						auto& instance_ref = std::any_cast<T&>(node_ptr->get_instance());
						if constexpr (std::string_view{f.name} == "move" && args.size() == 2) {
							if constexpr (std::is_invocable_v<decltype(f), T&, float, float>) {
								f(instance_ref, std::any_cast<float>(args[0]), std::any_cast<float>(args[1]));
							}
						} else if constexpr (std::string_view{f.name} == "print" && args.empty()) {
							if constexpr (std::is_invocable_v<decltype(f), const T&>) {
								f(instance_ref);
							}
						}
					};
				}
			});
			return node;
		};
		
		// Create the runtime PowerType object and store it in the registry.
		reg.emplace(type_name, PowerType(type_name, std::move(prop_getter), std::move(meth_getter), std::move(node_creator)));
	}
	
private:
	// The registry now stores the fully-formed PowerType objects.
	static std::map<std::string, PowerType>& get_registry();
};

// Helper to register a type at program startup.
template<typename T>
struct AutoRegistrator {
	AutoRegistrator() {
		ReflectionRegistry::register_type<T>();
	}
};

} // namespace power::reflection
