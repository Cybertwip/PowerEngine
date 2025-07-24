#pragma once

#include "serialization/UUID.hpp"
#include "refl.hpp"

#include <string>
#include <vector>
#include <map>
#include <string_view>
#include <functional>
#include <type_traits>
#include <memory>
#include <any>

#include "execution/ReflectedNode.hpp"

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
	std::vector<ParameterInfo> parameters;
};

// A type-erased, runtime-accessible wrapper for a reflected type.
class PowerType {
public:
	using NodeCreatorFunc = std::function<std::unique_ptr<CoreNode>(UUID)>;
	
private:
	std::string m_name;
	bool m_is_valid = false;
	std::function<std::vector<PropertyInfo>()> m_properties_getter;
	std::function<std::vector<MethodInfo>()> m_methods_getter;
	NodeCreatorFunc m_node_creator;
	
	PowerType() = default; // For creating an invalid type
	
public:
	PowerType(
			  std::string_view name,
			  std::function<std::vector<PropertyInfo>()> prop_getter,
			  std::function<std::vector<MethodInfo>()> meth_getter,
			  NodeCreatorFunc node_creator)
	: m_name(name),
	m_is_valid(true),
	m_properties_getter(std::move(prop_getter)),
	m_methods_getter(std::move(meth_getter)),
	m_node_creator(std::move(node_creator)) {}
	
	static PowerType get_by_name(const std::string& name);
	
	bool is_valid() const { return m_is_valid; }
	const std::string& get_name() const { return m_name; }
	
	std::vector<PropertyInfo> get_properties() const;
	std::vector<MethodInfo> get_methods() const;
	
	const NodeCreatorFunc& get_node_creator() const;
	bool has_node_creator() const;
	
	friend class ReflectionRegistry;
};

// A registry to hold all statically discovered types.
class ReflectionRegistry {
public:
	static std::vector<PowerType> get_all_types();
	static PowerType get_type_by_name(const std::string& name);
	
	template<typename T>
	static void register_type() {
		const std::string type_name(refl::reflect<T>().name);
		
		auto& reg = get_registry();
		if (reg.count(type_name)) {
			return; // Already registered
		}
		
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
		
		auto meth_getter = []() -> std::vector<MethodInfo> {
			std::vector<MethodInfo> meths;
			refl::util::for_each(refl::reflect<T>().members, [&](auto func) {
				if constexpr (refl::descriptor::is_function(func)) {
					MethodInfo mi;
					mi.name = std::string(func.name);
					
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
		
		auto node_creator = [](UUID id) -> std::unique_ptr<CoreNode> {
			PowerType type_info = PowerType::get_by_name(std::string(refl::reflect<T>().name));
			auto instance = std::make_any<T>();
			auto node = std::make_unique<ReflectedCoreNode>(id, type_info, instance);
			
			refl::util::for_each(refl::reflect<T>().members, [&](auto member) {
				if constexpr (refl::descriptor::is_field(member)) {
					std::string prop_name(member.name);
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
						auto& instance_ref = std::any_cast<T&>(node_ptr->get_instance());
						
						// FIX: Split compile-time and runtime checks.
						if constexpr (f.name == "move") {
							if (args.size() == 2) { // Runtime check
								if constexpr (std::is_invocable_v<decltype(f), T&, float, float>) {
									f(instance_ref, std::any_cast<float>(args[0]), std::any_cast<float>(args[1]));
								}
							}
						} else if constexpr (f.name == "print") {
							if (args.empty()) { // Runtime check
								if constexpr (std::is_invocable_v<decltype(f), const T&>) {
									f(instance_ref);
								}
							}
						}
					};
				}
			});
			return node;
		};
		
		reg.emplace(type_name, PowerType(type_name, std::move(prop_getter), std::move(meth_getter), std::move(node_creator)));
	}
	
private:
	// FIX: Implement the registry getter here using a static local (Meyers' Singleton).
	// This resolves the "incomplete type" error by ensuring the map is instantiated
	// only when first accessed, at which point PowerType is fully defined.
	static std::map<std::string, PowerType>& get_registry() {
		static std::map<std::string, PowerType> s_registry;
		return s_registry;
	}
};

// Helper to register a type at program startup.
template<typename T>
struct AutoRegistrator {
	AutoRegistrator() {
		ReflectionRegistry::register_type<T>();
	}
};


inline PowerType ReflectionRegistry::get_type_by_name(const std::string& name) {
	auto& reg = get_registry();
	if (auto it = reg.find(name); it != reg.end()) {
		return it->second; // Returns a copy
	}
	return {}; // Returns an invalid PowerType (uses private default constructor)
}

} // namespace power::reflection
