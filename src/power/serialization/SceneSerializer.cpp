#include "SceneSerializer.hpp"
#include "scene_generated.h" // Include the generated header

// Your actual component headers
#include "components/TransformComponent.hpp"
#include "components/CameraComponent.hpp"
#include "components/ModelMetadataComponent.hpp"
#include "components/BlueprintComponent.hpp"
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
void SceneSerializer::register_component<BlueprintComponent>() {
	entt::id_type type_id = entt::type_id<BlueprintComponent>().hash();
	
	m_serializers[type_id] = {
		// --- SERIALIZE ---
		.serialize = [](flatbuffers::FlatBufferBuilder& builder, const entt::registry& registry, entt::entity entity) {
			auto& bp_comp = registry.get<BlueprintComponent>(entity);
			auto& node_processor = bp_comp.node_processor();
			
			// 1. Serialize all nodes in the blueprint graph
			std::vector<flatbuffers::Offset<Power::Schema::BlueprintNode>> node_offsets;
			for(const auto& node_ptr : node_processor.get_nodes()) {
				const auto& node = *node_ptr;
				
				// Serialize input pins for the current node
				std::vector<flatbuffers::Offset<Power::Schema::BlueprintPin>> input_pin_offsets;
				for (const auto& pin_ptr : node.get_inputs()) {
					auto payload_offset = serialize_payload(builder, pin_ptr->get_data());
					auto pin_offset = Power::Schema::CreateBlueprintPin(builder, pin_ptr->id,
																		static_cast<Power::Schema::PinType>(pin_ptr->type),
																		static_cast<Power::Schema::PinSubType>(pin_ptr->subtype),
																		static_cast<Power::Schema::PinKind>(pin_ptr->kind), payload_offset);
					input_pin_offsets.push_back(pin_offset);
				}
				
				// Serialize output pins for the current node
				std::vector<flatbuffers::Offset<Power::Schema::BlueprintPin>> output_pin_offsets;
				for (const auto& pin_ptr : node.get_outputs()) {
					auto payload_offset = serialize_payload(builder, pin_ptr->get_data());
					auto pin_offset = Power::Schema::CreateBlueprintPin(builder, pin_ptr->id,
																		static_cast<Power::Schema::PinType>(pin_ptr->type),
																		static_cast<Power::Schema::PinSubType>(pin_ptr->subtype),
																		static_cast<Power::Schema::PinKind>(pin_ptr->kind), payload_offset);
					output_pin_offsets.push_back(pin_offset);
				}
				
				auto inputs_vec = builder.CreateVector(input_pin_offsets);
				auto outputs_vec = builder.CreateVector(output_pin_offsets);
				auto position = Power::Schema::Vec2i(node.position.x(), node.position.y());
				
				// Serialize node-specific data (for DataCoreNode derivatives)
				flatbuffers::Offset<Power::Schema::BlueprintPayload> node_data_offset = 0;
				if (auto* data_node = dynamic_cast<const DataCoreNode*>(&node)) {
					node_data_offset = serialize_payload(builder, data_node->get_data());
				}
				
				auto node_offset = Power::Schema::CreateBlueprintNode(builder, node.id,
																	  static_cast<Power::Schema::NodeType>(node.type), &position,
																	  inputs_vec, outputs_vec, node_data_offset);
				node_offsets.push_back(node_offset);
			}
			auto nodes_vector = builder.CreateVector(node_offsets);
			
			// 2. Serialize all links in the blueprint graph
			std::vector<flatbuffers::Offset<Power::Schema::BlueprintLink>> link_offsets;
			for (const auto& link_ptr : node_processor.get_links()) {
				auto link_offset = Power::Schema::CreateBlueprintLink(builder, link_ptr->get_id(),
																	  link_ptr->get_start().node.id, link_ptr->get_start().id,
																	  link_ptr->get_end().node.id, link_ptr->get_end().id);
				link_offsets.push_back(link_offset);
			}
			auto links_vector = builder.CreateVector(link_offsets);
			
			// 3. Create the final BlueprintComponent table
			auto bp_comp_offset = Power::Schema::CreateBlueprintComponent(builder, nodes_vector, links_vector);
			return bp_comp_offset.Union();
		},
		// --- DESERIALIZE ---
			.deserialize = [](entt::registry& registry, entt::entity entity, const void* data) {
				const auto* comp_data = static_cast<const Power::Schema::BlueprintComponent*>(data);
				auto node_processor = std::make_unique<NodeProcessor>();
				
				if (!comp_data->nodes()) {
					registry.emplace<BlueprintComponent>(entity, std::move(node_processor));
					return;
				}
				
				// Map pin IDs to their C++ objects for re-linking
				std::unordered_map<int, CorePin*> pin_map;
				
				// 1. Re-create all nodes and their pins
				for (const auto* fbs_node : *comp_data->nodes()) {
					CoreNode* new_node_ptr = nullptr;
					switch(static_cast<NodeType>(fbs_node->type())) {
						case NodeType::KeyPress:   new_node_ptr = &node_processor->spawn_node<KeyPressCoreNode>(fbs_node->id()); break;
						case NodeType::KeyRelease: new_node_ptr = &node_processor->spawn_node<KeyReleaseCoreNode>(fbs_node->id()); break;
						case NodeType::String:     new_node_ptr = &node_processor->spawn_node<StringCoreNode>(fbs_node->id()); break;
						case NodeType::Print:      new_node_ptr = &node_processor->spawn_node<PrintCoreNode>(fbs_node->id()); break;
					}
					
					if(!new_node_ptr) continue;
					
					new_node_ptr->set_position({fbs_node->position()->x(), fbs_node->position()->y()});
					
					// Restore node-specific data
					if (auto* data_node = dynamic_cast<DataCoreNode*>(new_node_ptr)) {
						data_node->set_data(deserialize_payload(fbs_node->data()));
					}
					
					// Restore pin data and populate the pin_map
					for (const auto* fbs_pin : *fbs_node->inputs()) {
						auto& pin = new_node_ptr->get_pin(fbs_pin->id());
						pin.set_data(deserialize_payload(fbs_pin->data()));
						pin_map[pin.id] = &pin;
					}
					for (const auto* fbs_pin : *fbs_node->outputs()) {
						auto& pin = new_node_ptr->get_pin(fbs_pin->id());
						pin.set_data(deserialize_payload(fbs_pin->data()));
						pin_map[pin.id] = &pin;
					}
				}
				
				// 2. Re-create all links between the restored pins
				if (comp_data->links()) {
					for (const auto* fbs_link : *comp_data->links()) {
						if (pin_map.count(fbs_link->start_pin_id()) && pin_map.count(fbs_link->end_pin_id())) {
							node_processor->create_link(fbs_link->id(), *pin_map.at(fbs_link->start_pin_id()), *pin_map.at(fbs_link->end_pin_id()));
						}
					}
				}
				
				registry.emplace<BlueprintComponent>(entity, std::move(node_processor));
			}
	};
	m_component_type_map[static_cast<uint8_t>(Power::Schema::ComponentData_BlueprintComponent)] = type_id;
}
// ✅ END: BLUEPRINT COMPONENT SERIALIZATION


// --- Main Serialization/Deserialization Logic (Unchanged) ---

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
