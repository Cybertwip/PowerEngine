#pragma once

#include <entt/entt.hpp>
#include <flatbuffers/flatbuffers.h>
#include <functional>
#include <unordered_map>

// Forward declare the generated schema root
namespace Power { namespace Schema { struct Scene; } }

// A struct to hold the functions for a specific component type
struct ComponentSerializer {
    // Writes a component to the FlatBuffer builder and returns its offset
    std::function<flatbuffers::Offset<void>(
        flatbuffers::FlatBufferBuilder&,
        const entt::registry&,
        entt::entity
    )> serialize;

    // Reads a component from the FlatBuffer and adds it to the entity
    std::function<void(
        entt::registry&,
        entt::entity,
        const void* // This will be a pointer to the FlatBuffer table (e.g., const Schema::TransformComponent*)
    )> deserialize;
};

class SceneSerializer {
public:
    // Register serialization/deserialization functions for a component type
    template<typename T>
    void register_component();

    // The main functions
    void serialize(entt::registry& registry, const std::string& filepath);
    void deserialize(entt::registry& registry, const std::string& filepath);

private:
    // Map from a component's type ID to its serializer functions
    std::unordered_map<entt::id_type, ComponentSerializer> m_serializers;
    
    // Map from the FlatBuffers enum value to a component's type ID
    std::unordered_map<uint8_t, entt::id_type> m_component_type_map;
};