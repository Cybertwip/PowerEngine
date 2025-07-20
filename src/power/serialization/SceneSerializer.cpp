#include "SceneSerializer.hpp"
#include "scene_generated.h" // Include the generated header

// Your actual component headers
#include "components/TransformComponent.hpp"
#include "components/CameraComponent.hpp"
#include "components/ModelMetadataComponent.hpp"
#include "components/BlueprintComponent.hpp"
#include "components/BlueprintMetadataComponent.hpp"
#include "components/MetadataComponent.hpp"
#include "execution/BlueprintNode.hpp"
#include "execution/KeyPressNode.hpp"
#include "execution/KeyReleaseNode.hpp"
#include "execution/StringNode.hpp"
#include "execution/PrintNode.hpp"

#include "execution/NodeProcessor.hpp"

#include "serialization/UUID.hpp"


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
	// This function will be specialized for each component type
	// If you try to register an unknown component, it should fail to compile
	static_assert(sizeof(T) == 0, "Component type not registered for serialization.");
}

// --- Specializations for each component ---

// TransformComponent Registration
template<>
void SceneSerializer::register_component<TransformComponent>() {
	entt::id_type type_id = entt::type_id<TransformComponent>().hash();
	
	m_serializers[type_id] = {
		// --- SERIALIZE ---
		.serialize = [](flatbuffers::FlatBufferBuilder& builder, const entt::registry& registry, entt::entity entity) {
			const auto& comp = registry.get<TransformComponent>(entity);
			auto translation = create_vec3(comp.get_translation());
			auto rotation = create_quat(comp.get_rotation());
			auto scale = create_vec3(comp.get_scale());
			
			auto transform_offset = Power::Schema::CreateTransformComponent(builder, &translation, &rotation, &scale);
			return transform_offset.Union();
		},
		// --- DESERIALIZE ---
			.deserialize = [](entt::registry& registry, entt::entity entity, const void* data) {
				const auto* comp_data = static_cast<const Power::Schema::TransformComponent*>(data);
				auto& comp = registry.emplace<TransformComponent>(entity);
				comp.set_translation({comp_data->translation()->x(), comp_data->translation()->y(), comp_data->translation()->z()});
				comp.set_rotation({comp_data->rotation()->w(), comp_data->rotation()->x(), comp_data->rotation()->y(), comp_data->rotation()->z()});
				comp.set_scale({comp_data->scale()->x(), comp_data->scale()->y(), comp_data->scale()->z()});
			}
	};
	m_component_type_map[static_cast<uint8_t>(Power::Schema::ComponentData_TransformComponent)] = type_id;
}


// CameraComponent Registration
template<>
void SceneSerializer::register_component<CameraComponent>() {
	entt::id_type type_id = entt::type_id<CameraComponent>().hash();
	
	m_serializers[type_id] = {
		// --- SERIALIZE ---
		.serialize = [](flatbuffers::FlatBufferBuilder& builder, const entt::registry& registry, entt::entity entity) {
			const auto& comp = registry.get<CameraComponent>(entity);
			auto camera_offset = Power::Schema::CreateCameraComponent(builder, comp.get_fov(), comp.get_near(), comp.get_far(), comp.get_aspect(), comp.active());
			return camera_offset.Union();
		},
		// --- DESERIALIZE ---
			.deserialize = [](entt::registry& registry, entt::entity entity, const void* data) {
				const auto* comp_data = static_cast<const Power::Schema::CameraComponent*>(data);
				auto& transform = registry.get<TransformComponent>(entity);
				auto& comp = registry.emplace<CameraComponent>(entity, transform, comp_data->fov(), comp_data->near(), comp_data->far(), comp_data->aspect());
				comp.set_active(comp_data->active());
			}
	};
	m_component_type_map[static_cast<uint8_t>(Power::Schema::ComponentData_CameraComponent)] = type_id;
}

// IDComponent Registration (no-op for serialization)
template<>
void SceneSerializer::register_component<IDComponent>() {
	m_serializers[entt::type_id<IDComponent>().hash()] = {
		.serialize = nullptr,
		.deserialize = nullptr
	};
}

// ModelMetadataComponent Registration
template<>
void SceneSerializer::register_component<ModelMetadataComponent>() {
	entt::id_type type_id = entt::type_id<ModelMetadataComponent>().hash();
	
	m_serializers[type_id] = {
		// --- SERIALIZE ---
		.serialize = [](flatbuffers::FlatBufferBuilder& builder, const entt::registry& registry, entt::entity entity) {
			const auto& comp = registry.get<ModelMetadataComponent>(entity);
			auto model_path_offset = builder.CreateString(comp.model_path());
			auto metadata_offset = Power::Schema::CreateModelMetadataComponent(builder, model_path_offset);
			return metadata_offset.Union();
		},
		// --- DESERIALIZE ---
			.deserialize = [](entt::registry& registry, entt::entity entity, const void* data) {
				const auto* comp_data = static_cast<const Power::Schema::ModelMetadataComponent*>(data);
				std::string model_path = comp_data->model_path()->str();
				registry.emplace<ModelMetadataComponent>(entity, model_path);
			}
	};
	
	m_component_type_map[static_cast<uint8_t>(Power::Schema::ComponentData_ModelMetadataComponent)] = type_id;
}


// ✅ START: BLUEPRINT COMPONENT SERIALIZATION
// Helper to serialize the std::variant data payload from a CorePin or DataCoreNode.
flatbuffers::Offset<Power::Schema::BlueprintPayload> serialize_payload(
																	   flatbuffers::FlatBufferBuilder& builder,
																	   const std::optional<std::variant<Entity, std::string, int, float, bool>>& data) {
	
	if (!data.has_value()) {
		return 0; // Return null offset if no data.
	}
	
	Power::Schema::BlueprintPayloadData payload_type = Power::Schema::BlueprintPayloadData_NONE;
	flatbuffers::Offset<void> payload_offset;
	
	std::visit([&](auto&& arg) {
		using T = std::decay_t<decltype(arg)>;
		if constexpr (std::is_same_v<T, std::string>) {
			payload_type = Power::Schema::BlueprintPayloadData_s;
			payload_offset = builder.CreateString(arg).Union();
		} else if constexpr (std::is_same_v<T, int>) {
			payload_type = Power::Schema::BlueprintPayloadData_i;
			payload_offset = Power::Schema::CreateIntVal(builder, arg).Union();
		} else if constexpr (std::is_same_v<T, float>) {
			payload_type = Power::Schema::BlueprintPayloadData_f;
			payload_offset = Power::Schema::CreateFloatVal(builder, arg).Union();
		} else if constexpr (std::is_same_v<T, bool>) {
			payload_type = Power::Schema::BlueprintPayloadData_b;
			payload_offset = Power::Schema::CreateBoolVal(builder, arg).Union();
		} else if constexpr (std::is_same_v<T, Entity>) {
			payload_type = Power::Schema::BlueprintPayloadData_e;
			payload_offset = Power::Schema::CreateBlueprintEntityPayload(builder, arg.id).Union();
		}
	}, data.value());
	
	if (payload_offset.IsNull()) {
		return 0;
	}
	
	return Power::Schema::CreateBlueprintPayload(builder, payload_type, payload_offset);
}

// Helper to deserialize a FlatBuffer payload back into a std::variant.
std::optional<std::variant<Entity, std::string, int, float, bool>> deserialize_payload(
																					   const Power::Schema::BlueprintPayload* payload_data) {
	
	if (!payload_data || payload_data->data_type() == Power::Schema::BlueprintPayloadData_NONE) {
		return std::nullopt;
	}
	
	switch (payload_data->data_type()) {
		case Power::Schema::BlueprintPayloadData_s:
			return payload_data->data_as_s()->str();
		case Power::Schema::BlueprintPayloadData_i:
			return payload_data->data_as_i()->val();
		case Power::Schema::BlueprintPayloadData_f:
			return payload_data->data_as_f()->val();
		case Power::Schema::BlueprintPayloadData_b:
			return payload_data->data_as_b()->val();
		case Power::Schema::BlueprintPayloadData_e:
			return Entity{payload_data->data_as_e()->id(), std::nullopt};
		default:
			return std::nullopt;
	}
}

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
	
	m_component_type_map[static_cast<uint8_t>(Power::Schema::ComponentData_BlueprintMetadataComponent)] = type_id;
}

// MetadataComponent Registration
template<>
void SceneSerializer::register_component<MetadataComponent>() {
	entt::id_type type_id = entt::type_id<MetadataComponent>().hash();
	
	m_serializers[type_id] = {
		// --- SERIALIZE ---
		.serialize = [](flatbuffers::FlatBufferBuilder& builder, const entt::registry& registry, entt::entity entity) {
			const auto& comp = registry.get<MetadataComponent>(entity);
			auto name_offset = builder.CreateString(std::string(comp.name()));
			auto metadata_offset = Power::Schema::CreateMetadataComponent(builder, comp.identifier(), name_offset);
			return metadata_offset.Union();
		},
		// --- DESERIALIZE ---
			.deserialize = [](entt::registry& registry, entt::entity entity, const void* data) {
				const auto* comp_data = static_cast<const Power::Schema::MetadataComponent*>(data);
				std::string name = comp_data->name()->str();
				registry.emplace<MetadataComponent>(entity, comp_data->identifier(), name);
			}
	};
	
	// Assumes 'MetadataComponent' is defined in your FlatBuffers schema and the corresponding enum is 'ComponentData_MetadataComponent'
	m_component_type_map[static_cast<uint8_t>(Power::Schema::ComponentData_MetadataComponent)] = type_id;
}


void SceneSerializer::serialize(entt::registry& registry, const std::string& filepath) {
	flatbuffers::FlatBufferBuilder builder;
	std::vector<flatbuffers::Offset<Power::Schema::Entity>> entity_offsets;
	
	auto id_view = registry.view<const IDComponent>();
	for (auto entity_handle : id_view) {
		std::vector<flatbuffers::Offset<Power::Schema::Component>> component_offsets;
		
		for (auto&& [type_id, storage] : registry.storage()) {
			if (type_id != entt::type_id<IDComponent>().hash() && storage.contains(entity_handle) && m_serializers.count(type_id)) {
				auto& serializer = m_serializers.at(type_id);
				
				if (serializer.serialize) {
					auto component_offset = serializer.serialize(builder, registry, entity_handle);
					
					Power::Schema::ComponentData component_type_enum = Power::Schema::ComponentData_NONE;
					for(const auto& [key, val] : m_component_type_map) {
						if (val == type_id) {
							component_type_enum = static_cast<Power::Schema::ComponentData>(key);
							break;
						}
					}
					
					if (component_type_enum != Power::Schema::ComponentData_NONE) {
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

void SceneSerializer::deserialize(entt::registry& registry, const std::string& filepath) {
	std::ifstream ifs(filepath, std::ios::binary | std::ios::ate);
	if (!ifs.is_open()) return;
	
	auto size = ifs.tellg();
	ifs.seekg(0, std::ios::beg);
	std::vector<char> buffer(size);
	if(size == 0) return;
	ifs.read(buffer.data(), size);
	
	registry.clear();
	
	auto scene = Power::Schema::GetScene(buffer.data());
	if (!scene->entities()) return;
	
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
