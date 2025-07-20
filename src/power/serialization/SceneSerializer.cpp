#include "SceneSerializer.hpp"
#include "scene_generated.h"

// Your actual component headers
#include "components/TransformComponent.hpp"
#include "components/CameraComponent.hpp"
#include "components/ModelMetadataComponent.hpp"
#include "components/BlueprintComponent.hpp"
#include "components/BlueprintMetadataComponent.hpp"
#include "components/MetadataComponent.hpp"
#include "serialization/UUID.hpp" // Make sure IDComponent is included

#include <fstream>
#include <vector>
#include <unordered_map>

// Helper to convert our Vec3 to the schema's Vec3
Power::Schema::Vec3 create_vec3(const glm::vec3& v) {
	return Power::Schema::Vec3(v.x, v.y, v.z);
}

// Helper to convert our Quat to the schema's Quat
Power::Schema::Quat create_quat(const glm::quat& q) {
	return Power::Schema::Quat(q.w, q.x, q.y, q.z);
}

// We define the registration logic inside the .cpp file
template<typename T>
void SceneSerializer::register_component() {
	static_assert(sizeof(T) == 0, "Component type not registered for serialization.");
}

// --- Helper macro to reduce registration boilerplate ---
#define REGISTER_COMPONENT(TYPE, ENUM) \
template<> \
void SceneSerializer::register_component<TYPE>() { \
entt::id_type type_id = entt::type_id<TYPE>().hash(); \
m_serializers[type_id] = { \
.serialize = [](flatbuffers::FlatBufferBuilder& builder, const entt::registry& registry, entt::entity entity) { \
const auto& comp = registry.get<TYPE>(entity); \
return comp.serialize(builder); \
}, \
.deserialize = [](entt::registry& registry, entt::entity entity, const void* data) { \
TYPE::deserialize(registry, entity, data); \
} \
}; \
auto enum_val = static_cast<uint8_t>(Power::Schema::ComponentData_##ENUM); \
m_component_type_map[enum_val] = type_id; \
m_type_id_to_enum_map[type_id] = enum_val; /* MODIFIED: Populate reverse map */ \
}

// To use the macro, you'll need to add serialize/deserialize static methods to your components.
// For example, in TransformComponent.hpp/cpp:
/*
 struct TransformComponent {
 // ... existing members and methods
 public:
 static flatbuffers::Offset<void> serialize(flatbuffers::FlatBufferBuilder& builder, const TransformComponent& comp) {
 auto translation = create_vec3(comp.get_translation());
 auto rotation = create_quat(comp.get_rotation());
 auto scale = create_vec3(comp.get_scale());
 auto offset = Power::Schema::CreateTransformComponent(builder, &translation, &rotation, &scale);
 return offset.Union();
 }
 static void deserialize(entt::registry& registry, entt::entity entity, const void* data) {
 const auto* comp_data = static_cast<const Power::Schema::TransformComponent*>(data);
 auto& comp = registry.emplace<TransformComponent>(entity);
 comp.set_translation({comp_data->translation()->x(), comp_data->translation()->y(), comp_data->translation()->z()});
 comp.set_rotation({comp_data->rotation()->w(), comp_data->rotation()->x(), comp_data->rotation()->y(), comp_data->rotation()->z()});
 comp.set_scale({comp_data->scale()->x(), comp_data->scale()->y(), comp_data->scale()->z()});
 }
 };
 */
// You would do this for all serializable components. For brevity, the original long-form specializations are kept below.

// --- Specializations for each component ---

// TransformComponent Registration
template<>
void SceneSerializer::register_component<TransformComponent>() {
	entt::id_type type_id = entt::type_id<TransformComponent>().hash();
	m_serializers[type_id] = {
		.serialize = [](flatbuffers::FlatBufferBuilder& builder, const entt::registry& registry, entt::entity entity) {
			const auto& comp = registry.get<TransformComponent>(entity);
			auto translation = create_vec3(comp.get_translation());
			auto rotation = create_quat(comp.get_rotation());
			auto scale = create_vec3(comp.get_scale());
			auto transform_offset = Power::Schema::CreateTransformComponent(builder, &translation, &rotation, &scale);
			return transform_offset.Union();
		},
			.deserialize = [](entt::registry& registry, entt::entity entity, const void* data) {
				const auto* comp_data = static_cast<const Power::Schema::TransformComponent*>(data);
				auto& comp = registry.emplace<TransformComponent>(entity);
				comp.set_translation({comp_data->translation()->x(), comp_data->translation()->y(), comp_data->translation()->z()});
				comp.set_rotation({comp_data->rotation()->w(), comp_data->rotation()->x(), comp_data->rotation()->y(), comp_data->rotation()->z()});
				comp.set_scale({comp_data->scale()->x(), comp_data->scale()->y(), comp_data->scale()->z()});
			}
	};
	auto enum_val = static_cast<uint8_t>(Power::Schema::ComponentData_TransformComponent);
	m_component_type_map[enum_val] = type_id;
	m_type_id_to_enum_map[type_id] = enum_val; // MODIFIED
}

// CameraComponent Registration
template<>
void SceneSerializer::register_component<CameraComponent>() {
	entt::id_type type_id = entt::type_id<CameraComponent>().hash();
	m_serializers[type_id] = {
		.serialize = [](flatbuffers::FlatBufferBuilder& builder, const entt::registry& registry, entt::entity entity) {
			const auto& comp = registry.get<CameraComponent>(entity);
			auto camera_offset = Power::Schema::CreateCameraComponent(builder, comp.get_fov(), comp.get_near(), comp.get_far(), comp.get_aspect(), comp.active());
			return camera_offset.Union();
		},
			.deserialize = [](entt::registry& registry, entt::entity entity, const void* data) {
				const auto* comp_data = static_cast<const Power::Schema::CameraComponent*>(data);
				auto& transform = registry.get<TransformComponent>(entity);
				auto& comp = registry.emplace<CameraComponent>(entity, transform, comp_data->fov(), comp_data->near(), comp_data->far(), comp_data->aspect());
				comp.set_active(comp_data->active());
			}
	};
	auto enum_val = static_cast<uint8_t>(Power::Schema::ComponentData_CameraComponent);
	m_component_type_map[enum_val] = type_id;
	m_type_id_to_enum_map[type_id] = enum_val; // MODIFIED
}

// IDComponent Registration (no-op for serialization)
template<>
void SceneSerializer::register_component<IDComponent>() {
	m_serializers[entt::type_id<IDComponent>().hash()] = { .serialize = nullptr, .deserialize = nullptr };
}

// ModelMetadataComponent Registration
template<>
void SceneSerializer::register_component<ModelMetadataComponent>() {
	entt::id_type type_id = entt::type_id<ModelMetadataComponent>().hash();
	m_serializers[type_id] = {
		.serialize = [](flatbuffers::FlatBufferBuilder& builder, const entt::registry& registry, entt::entity entity) {
			const auto& comp = registry.get<ModelMetadataComponent>(entity);
			auto model_path_offset = builder.CreateString(comp.model_path());
			auto metadata_offset = Power::Schema::CreateModelMetadataComponent(builder, model_path_offset);
			return metadata_offset.Union();
		},
			.deserialize = [](entt::registry& registry, entt::entity entity, const void* data) {
				const auto* comp_data = static_cast<const Power::Schema::ModelMetadataComponent*>(data);
				std::string model_path = comp_data->model_path()->str();
				registry.emplace<ModelMetadataComponent>(entity, model_path);
			}
	};
	auto enum_val = static_cast<uint8_t>(Power::Schema::ComponentData_ModelMetadataComponent);
	m_component_type_map[enum_val] = type_id;
	m_type_id_to_enum_map[type_id] = enum_val; // MODIFIED
}

// BlueprintMetadataComponent Registration
template<>
void SceneSerializer::register_component<BlueprintMetadataComponent>() {
	entt::id_type type_id = entt::type_id<BlueprintMetadataComponent>().hash();
	m_serializers[type_id] = {
		.serialize = [](flatbuffers::FlatBufferBuilder& builder, const entt::registry& registry, entt::entity entity) {
			const auto& comp = registry.get<BlueprintMetadataComponent>(entity);
			auto path_offset = builder.CreateString(comp.blueprint_path());
			auto metadata_offset = Power::Schema::CreateBlueprintMetadataComponent(builder, path_offset);
			return metadata_offset.Union();
		},
			.deserialize = [](entt::registry& registry, entt::entity entity, const void* data) {
				const auto* comp_data = static_cast<const Power::Schema::BlueprintMetadataComponent*>(data);
				std::string path = comp_data->blueprint_path()->str();
				registry.emplace<BlueprintMetadataComponent>(entity, path);
			}
	};
	auto enum_val = static_cast<uint8_t>(Power::Schema::ComponentData_BlueprintMetadataComponent);
	m_component_type_map[enum_val] = type_id;
	m_type_id_to_enum_map[type_id] = enum_val; // MODIFIED
}

// MetadataComponent Registration
template<>
void SceneSerializer::register_component<MetadataComponent>() {
	entt::id_type type_id = entt::type_id<MetadataComponent>().hash();
	m_serializers[type_id] = {
		.serialize = [](flatbuffers::FlatBufferBuilder& builder, const entt::registry& registry, entt::entity entity) {
			const auto& comp = registry.get<MetadataComponent>(entity);
			auto name_offset = builder.CreateString(std::string(comp.name()));
			auto metadata_offset = Power::Schema::CreateMetadataComponent(builder, comp.identifier(), name_offset);
			return metadata_offset.Union();
		},
			.deserialize = [](entt::registry& registry, entt::entity entity, const void* data) {
				const auto* comp_data = static_cast<const Power::Schema::MetadataComponent*>(data);
				std::string name = comp_data->name()->str();
				registry.emplace<MetadataComponent>(entity, comp_data->identifier(), name);
			}
	};
	auto enum_val = static_cast<uint8_t>(Power::Schema::ComponentData_MetadataComponent);
	m_component_type_map[enum_val] = type_id;
	m_type_id_to_enum_map[type_id] = enum_val; // MODIFIED
}

// =================================================================
//                    FIXED SERIALIZE FUNCTION
// =================================================================
void SceneSerializer::serialize(entt::registry& registry, const std::string& filepath) {
	flatbuffers::FlatBufferBuilder builder;
	std::vector<flatbuffers::Offset<Power::Schema::Entity>> entity_offsets;
	
	auto id_view = registry.view<const IDComponent>();
	for (auto entity_handle : id_view) {
		std::vector<flatbuffers::Offset<Power::Schema::Component>> component_offsets;
		
		// MODIFIED: Reverted to this loop for compatibility with older EnTT versions.
		for (auto&& [type_id, storage] : registry.storage()) {
			// Check if the current entity actually has this component.
			if (storage.contains(entity_handle)) {
				// Ensure a serializer is registered and it's not a no-op (like for IDComponent).
				if (m_serializers.count(type_id) && m_serializers.at(type_id).serialize) {
					
					// KEEPING THE FIX: Use the efficient reverse map to find the enum type.
					if (m_type_id_to_enum_map.count(type_id)) {
						auto& serializer = m_serializers.at(type_id);
						auto component_offset = serializer.serialize(builder, registry, entity_handle);
						auto component_type_enum = static_cast<Power::Schema::ComponentData>(m_type_id_to_enum_map.at(type_id));
						component_offsets.push_back(Power::Schema::CreateComponent(builder, component_type_enum, component_offset));
					}
				}
			}
		}
		
		if (!component_offsets.empty()) {
			UUID uuid = id_view.get<const IDComponent>(entity_handle).uuid;
			auto components_vector = builder.CreateVector(component_offsets);
			auto entity_offset = Power::Schema::CreateEntity(builder, uuid, components_vector);
			entity_offsets.push_back(entity_offset);
		}
	}
	
	auto entities_vector = builder.CreateVector(entity_offsets);
	auto scene_offset = Power::Schema::CreateScene(builder, entities_vector);
	builder.Finish(scene_offset);
	
	std::ofstream ofs(filepath, std::ios::binary);
	ofs.write(reinterpret_cast<const char*>(builder.GetBufferPointer()), builder.GetSize());
}

// Your deserialize function was already correct, but here it is for completeness.
void SceneSerializer::deserialize(entt::registry& registry, const std::string& filepath) {
	std::ifstream ifs(filepath, std::ios::binary | std::ios::ate);
	if (!ifs.is_open()) return;
	
	auto size = ifs.tellg();
	if(size == 0) return;
	ifs.seekg(0, std::ios::beg);
	std::vector<char> buffer(size);
	ifs.read(buffer.data(), size);
	
	registry.clear();
	
	auto scene = Power::Schema::GetScene(buffer.data());
	if (!scene || !scene->entities()) return;
	
	std::unordered_map<UUID, entt::entity> uuid_map;
	
	// First pass: create all entities and give them an IDComponent
	for (const auto* entity_data : *scene->entities()) {
		UUID uuid = entity_data->uuid();
		auto new_entity = registry.create();
		registry.emplace<IDComponent>(new_entity, uuid);
		uuid_map[uuid] = new_entity;
	}
	
	// Second pass: deserialize and emplace all other components
	for (const auto* entity_data : *scene->entities()) {
		if (!entity_data->components()) continue;
		
		UUID uuid = entity_data->uuid();
		entt::entity entity_handle = uuid_map.at(uuid);
		
		for (const auto* component_data : *entity_data->components()) {
			auto component_type_enum = component_data->data_type();
			if (m_component_type_map.count(static_cast<uint8_t>(component_type_enum))) {
				entt::id_type type_id = m_component_type_map.at(static_cast<uint8_t>(component_type_enum));
				auto& serializer = m_serializers.at(type_id);
				
				if(serializer.deserialize) {
					serializer.deserialize(registry, entity_handle, component_data->data());
				}
			}
		}
	}
}
