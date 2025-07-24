/*
 * =====================================================================================
 *
 * Filename:  ReflectedNode.hpp
 *
 * Description:  A generic blueprint node that represents any C++ type
 * exposed to the power::reflection system.
 *
 * =====================================================================================
 */
#pragma once

#include "BlueprintNode.hpp"

#include <any>
#include <functional>
#include <string>
#include <map>
#include <stdexcept>

namespace power::reflection {
	class PowerType;
	class ReflectionRegistry;
}

class BlueprintCanvas;

// A helper to map reflected type names to our blueprint PinType enum.
inline PinType string_to_pin_type(const std::string& type_name) {
    if (type_name == "float") return PinType::Float;
    if (type_name == "int" || type_name == "long long") return PinType::Int;
    if (type_name == "bool") return PinType::Bool;
    if (type_name == "std::string") return PinType::String;
    // Add other type mappings as needed
    return PinType::Object; // Default fallback
}


/**
 * @class ReflectedCoreNode
 * @brief The logical part of a generic, reflection-based blueprint node.
 *
 * This node holds an instance of a C++ object and uses reflection metadata
 * to generate its pins and define its behavior.
 */
class ReflectedCoreNode : public CoreNode {
public:
    /**
     * @brief Constructs a ReflectedCoreNode for a specific reflected type.
     * @param id The unique identifier for this node.
     * @param type_info The reflection metadata for the C++ type this node represents.
     * @param instance An std::any holding an actual instance of the C++ object.
     */
    ReflectedCoreNode(UUID id, const power::reflection::PowerType& type_info, std::any instance);

    /**
     * @brief Executes a method on the internal C++ object.
     *
     * This override is called when an execution flow pin is triggered. It
     * identifies which method to call, gathers data from the input pins,
     * and invokes the actual C++ function.
     * @param flow_pin_id The ID of the flow pin that triggered the execution.
     * @return True to allow the execution flow to continue.
     */
    bool evaluate(UUID flow_pin_id) override;

    // We need a way to get and set the internal object's property values.
    std::optional<std::any> get_property_value(const std::string& property_name);
    void set_property_value(const std::string& property_name, std::any value);

    const power::reflection::PowerType& get_type_info() const { return *m_type_info; }
    std::any& get_instance() { return m_object_instance; }

private:
    std::unique_ptr<power::reflection::PowerType> m_type_info;
    std::any m_object_instance;

    // A map from a method's flow-input-pin ID to its name.
    std::map<UUID, std::string> m_pin_to_method_name;

    // A map from a property name to its output pin.
    std::map<std::string, CorePin*> m_property_pins;

    // A map from a method name to its parameter input pins.
    std::map<std::string, std::vector<CorePin*>> m_method_param_pins;
    
    // A type-erased way to get a property's value from the std::any instance.
    std::map<std::string, std::function<std::any(std::any&)>> m_property_getters;
    
    // A type-erased way to set a property's value.
    std::map<std::string, std::function<void(std::any&, std::any)>> m_property_setters;

    // The dispatcher to call the actual C++ methods.
    // The key is the method name, the value is a function that takes arguments as a vector of std::any.
    std::map<std::string, std::function<void(std::vector<std::any>)>> m_method_dispatchers;

    // Allow the Visual node to access private members for UI hookups.
    friend class ReflectedVisualNode;
    
    // The factory in NodeProcessor needs to register the dispatchers.
    template<typename T>
    friend class NodeFactory;
	
	friend class power::reflection::ReflectionRegistry;
};


/**
 * @class ReflectedVisualNode
 * @brief The visual representation of a ReflectedCoreNode.
 *
 * It dynamically creates UI widgets (e.g., text boxes) for the properties
 * of the reflected C++ object, allowing users to edit them in the blueprint.
 */
class ReflectedVisualNode : public VisualBlueprintNode {
public:
    ReflectedVisualNode(BlueprintCanvas& parent, nanogui::Vector2i position, ReflectedCoreNode& coreNode);
};
