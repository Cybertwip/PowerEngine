#include "serialization/BlueprintSerializer.hpp"
#include "blueprint_generated.h" // Assumes flatc generated this header

#include "execution/NodeProcessor.hpp"
#include "execution/BlueprintNode.hpp"
#include "execution/KeyPressNode.hpp"
#include "execution/KeyReleaseNode.hpp"
#include "execution/StringNode.hpp"
#include "execution/PrintNode.hpp"
#include "serialization/UUID.hpp" // For the Entity struct

#include <fstream>
#include <vector>
#include <unordered_map>
#include <variant>
#include <optional>

namespace { // Anonymous namespace for local helpers

// Helper to serialize the std::variant data payload
flatbuffers::Offset<Power::Schema::BlueprintPayload> serialize_payload(
    flatbuffers::FlatBufferBuilder& builder,
    const std::optional<std::variant<Entity, std::string, int, float, bool>>& data) {

    if (!data.has_value()) {
        return 0;
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

    return payload_offset.IsNull() ? 0 : Power::Schema::CreateBlueprintPayload(builder, payload_type, payload_offset);
}

// Helper to deserialize a FlatBuffer payload back into a std::variant.
std::optional<std::variant<Entity, std::string, int, float, bool>> deserialize_payload(
    const Power::Schema::BlueprintPayload* payload_data) {
    if (!payload_data || payload_data->data_type() == Power::Schema::BlueprintPayloadData_NONE) {
        return std::nullopt;
    }
    switch (payload_data->data_type()) {
        case Power::Schema::BlueprintPayloadData_s: return payload_data->data_as_s()->str();
        case Power::Schema::BlueprintPayloadData_i: return payload_data->data_as_i()->val();
        case Power::Schema::BlueprintPayloadData_f: return payload_data->data_as_f()->val();
        case Power::Schema::BlueprintPayloadData_b: return payload_data->data_as_b()->val();
        case Power::Schema::BlueprintPayloadData_e: return Entity{payload_data->data_as_e()->id(), std::nullopt};
        default: return std::nullopt;
    }
}
} // namespace

void BlueprintSerializer::serialize(const NodeProcessor& node_processor, const std::string& filepath) {
    flatbuffers::FlatBufferBuilder builder;

    std::vector<flatbuffers::Offset<Power::Schema::BlueprintNode>> node_offsets;
    for (const auto& node_ptr : node_processor.get_nodes()) {
        const auto& node = *node_ptr;
        std::vector<flatbuffers::Offset<Power::Schema::BlueprintPin>> inputs, outputs;
        for (const auto& pin : node.get_inputs()) {
            inputs.push_back(Power::Schema::CreateBlueprintPin(builder, pin->id, (Power::Schema::PinType)pin->type, (Power::Schema::PinSubType)pin->subtype, (Power::Schema::PinKind)pin->kind, serialize_payload(builder, pin->get_data())));
        }
        for (const auto& pin : node.get_outputs()) {
            outputs.push_back(Power::Schema::CreateBlueprintPin(builder, pin->id, (Power::Schema::PinType)pin->type, (Power::Schema::PinSubType)pin->subtype, (Power::Schema::PinKind)pin->kind, serialize_payload(builder, pin->get_data())));
        }
        auto pos = Power::Schema::Vec2i(node.position.x(), node.position.y());
        flatbuffers::Offset<Power::Schema::BlueprintPayload> node_data_offset = 0;
        if (auto* data_node = dynamic_cast<const DataCoreNode*>(&node)) {
            node_data_offset = serialize_payload(builder, data_node->get_data());
        }
        node_offsets.push_back(Power::Schema::CreateBlueprintNode(builder, node.id, (Power::Schema::NodeType)node.type, &pos, builder.CreateVector(inputs), builder.CreateVector(outputs), node_data_offset));
    }

    std::vector<flatbuffers::Offset<Power::Schema::BlueprintLink>> link_offsets;
    for (const auto& link : node_processor.get_links()) {
        link_offsets.push_back(Power::Schema::CreateBlueprintLink(builder, link->get_id(), link->get_start().node.id, link->get_start().id, link->get_end().node.id, link->get_end().id));
    }

    auto blueprint_offset = Power::Schema::CreateBlueprint(builder, builder.CreateVector(node_offsets), builder.CreateVector(link_offsets));
    builder.Finish(blueprint_offset);

    std::ofstream ofs(filepath, std::ios::binary);
    ofs.write(reinterpret_cast<const char*>(builder.GetBufferPointer()), builder.GetSize());
}

void BlueprintSerializer::deserialize(NodeProcessor& node_processor, const std::string& filepath) {
    std::ifstream ifs(filepath, std::ios::binary | std::ios::ate);
    if (!ifs.is_open()) return;
    auto size = ifs.tellg();
    if (size == 0) return;
    ifs.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    ifs.read(buffer.data(), size);

    node_processor.clear();
    auto blueprint = Power::Schema::GetBlueprint(buffer.data());
    if (!blueprint || !blueprint->nodes()) return;

    std::unordered_map<int, CorePin*> pin_map;
    for (const auto* fbs_node : *blueprint->nodes()) {
        CoreNode* new_node = nullptr;
        switch ((NodeType)fbs_node->type()) {
            case NodeType::KeyPress:   new_node = &node_processor.spawn_node<KeyPressCoreNode>(fbs_node->id()); break;
            case NodeType::KeyRelease: new_node = &node_processor.spawn_node<KeyReleaseCoreNode>(fbs_node->id()); break;
            case NodeType::String:     new_node = &node_processor.spawn_node<StringCoreNode>(fbs_node->id()); break;
            case NodeType::Print:      new_node = &node_processor.spawn_node<PrintCoreNode>(fbs_node->id()); break;
        }
        if (!new_node) continue;
        new_node->set_position({fbs_node->position()->x(), fbs_node->position()->y()});
        if (auto* data_node = dynamic_cast<DataCoreNode*>(new_node)) {
            data_node->set_data(deserialize_payload(fbs_node->data()));
        }
        for (const auto* fbs_pin : *fbs_node->inputs()) {
            auto& pin = new_node->get_pin(fbs_pin->id());
            pin.set_data(deserialize_payload(fbs_pin->data()));
            pin_map[pin.id] = &pin;
        }
        for (const auto* fbs_pin : *fbs_node->outputs()) {
            auto& pin = new_node->get_pin(fbs_pin->id());
            pin.set_data(deserialize_payload(fbs_pin->data()));
            pin_map[pin.id] = &pin;
        }
    }

    if (blueprint->links()) {
        for (const auto* fbs_link : *blueprint->links()) {
            if (pin_map.count(fbs_link->start_pin_id()) && pin_map.count(fbs_link->end_pin_id())) {
                node_processor.create_link(fbs_link->id(), *pin_map.at(fbs_link->start_pin_id()), *pin_map.at(fbs_link->end_pin_id()));
            }
        }
    }
}