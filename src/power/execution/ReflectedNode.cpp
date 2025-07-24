#include "ReflectedNode.hpp"
#include "execution/BlueprintCanvas.hpp" // For on_modified()
#include <iostream>
#include <nanogui/textbox.h>
#include <nanogui/label.h>

ReflectedCoreNode::ReflectedCoreNode(UUID id, const power::reflection::PowerType& type_info, std::any instance)
    : CoreNode(NodeType::Reflected, id, nanogui::Color(210, 210, 210, 255)),
      m_type_info(type_info),
      m_object_instance(std::move(instance))
{
    // Store the reflected type name for serialization.
    reflected_type_name = m_type_info.get_name();

    // 1. Create Output Pins for each reflected PROPERTY
    for (const auto& prop_info : m_type_info.get_properties()) {
        PinType pin_type = string_to_pin_type(prop_info.type_name);
        CorePin& output_pin = add_output(pin_type, prop_info.name);
        m_property_pins[prop_info.name] = &output_pin;
    }

    // 2. Create Input Pins for each reflected METHOD
    // An input flow pin to trigger the method, and data pins for its parameters.
    for (const auto& method_info : m_type_info.get_methods()) {
        CorePin& flow_pin = add_input(PinType::Flow, method_info.name);
        m_pin_to_method_name[flow_pin.id] = method_info.name;

        // Store parameter pins for when we evaluate.
        std::vector<CorePin*> param_pins;
        for(const auto& param_info : method_info.parameters) {
            PinType pin_type = string_to_pin_type(param_info.type_name);
            // Label the pin with the parameter index since names aren't available.
            std::string pin_label = "param_" + std::to_string(param_info.index);
            CorePin& param_pin = add_input(pin_type, pin_label);
            param_pins.push_back(&param_pin);
        }
        m_method_param_pins[method_info.name] = param_pins;
    }

    // Always add one flow output to chain execution
    add_output(PinType::Flow, "then");
}

bool ReflectedCoreNode::evaluate(UUID flow_pin_id) {
    // 1. Update all property output pins with the current values from the object instance.
    for (auto const& [name, getter] : m_property_getters) {
        if (m_property_pins.count(name)) {
            m_property_pins[name]->set_data(getter(m_object_instance));
        }
    }
    
    // 2. Find which method was called via the flow pin ID.
    if (m_pin_to_method_name.find(flow_pin_id) == m_pin_to_method_name.end()) {
        std::cerr << "Error: Fired an unknown execution pin on node " << id << std::endl;
        return false; // Stop execution
    }
    const std::string& method_to_call = m_pin_to_method_name.at(flow_pin_id);

    // 3. Gather arguments from the method's corresponding parameter pins.
    std::vector<std::any> args;
    if (m_method_param_pins.count(method_to_call)) {
        for (CorePin* param_pin : m_method_param_pins.at(method_to_call)) {
            args.push_back(param_pin->get_data().value_or(std::any{})); // Pass empty 'any' if not connected
        }
    }

    // 4. Use the dispatcher to invoke the real C++ method.
    if (m_method_dispatchers.count(method_to_call)) {
        std::cout << "Blueprint: Calling " << m_type_info.get_name() << "::" << method_to_call << "..." << std::endl;
        m_method_dispatchers.at(method_to_call)(args);
    } else {
        std::cerr << "Error: No dispatcher registered for method: " << method_to_call << std::endl;
        return false;
    }

    return true; // Allow execution to continue
}


std::optional<std::any> ReflectedCoreNode::get_property_value(const std::string& property_name) {
    if (m_property_getters.count(property_name)) {
        return m_property_getters.at(property_name)(m_object_instance);
    }
    return std::nullopt;
}

void ReflectedCoreNode::set_property_value(const std::string& property_name, std::any value) {
    if (m_property_setters.count(property_name)) {
        m_property_setters.at(property_name)(m_object_instance, std::move(value));
    }
}


// --- Visual Node Implementation ---

ReflectedVisualNode::ReflectedVisualNode(BlueprintCanvas& parent, nanogui::Vector2i position, ReflectedCoreNode& coreNode)
: VisualBlueprintNode(parent, coreNode.get_type_info().get_name(), position, nanogui::Vector2i(250, 150), coreNode) {
    
    // Dynamically create UI widgets for each editable property.
    for (const auto& prop : coreNode.get_type_info().get_properties()) {
        // Add a label for the property name.
        add_data_widget<nanogui::Label>(prop.name + ":", "sans-bold");
        
        // Add a text box to edit it.
        auto& textbox = add_data_widget<nanogui::TextBox>();
        textbox.set_editable(true);
        
        // Populate the textbox with the initial value from the C++ object.
        auto initial_value = coreNode.get_property_value(prop.name);
        if (initial_value) {
            if (prop.type_name == "float") {
                 textbox.set_value(std::to_string(std::any_cast<float>(*initial_value)));
                 textbox.set_format("[0-9]*\\.?[0-9]+");
            }
             else if (prop.type_name == "int") {
                 textbox.set_value(std::to_string(std::any_cast<int>(*initial_value)));
                 textbox.set_format("[\\-]?[0-9]+");
            }
            // Add more types as needed...
        }

        // When the user changes the text, update the underlying C++ object.
        textbox.set_callback([this, &coreNode, prop_name = prop.name, prop_type = prop.type_name](const std::string& val) {
            try {
                if (prop_type == "float") {
                    coreNode.set_property_value(prop_name, std::stof(val));
                } else if (prop_type == "int") {
                    coreNode.set_property_value(prop_name, std::stoi(val));
                }
                // Add more types as needed...
                 mCanvas.on_modified();
            } catch (const std::invalid_argument& e) {
                // Ignore invalid input
            }
            return true;
        });
    }
}