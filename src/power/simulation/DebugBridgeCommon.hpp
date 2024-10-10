#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include <stdexcept>

class DebugCommand {
public:
    enum class CommandType : uint32_t {
        EXECUTE = 1,
        RESPONSE = 2
    };

    CommandType type;
    std::string payload;

    DebugCommand() : type(CommandType::EXECUTE), payload("") {}
    DebugCommand(CommandType cmd_type, const std::string& data)
        : type(cmd_type), payload(data) {}

    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> bytes;

        uint32_t cmd_type = static_cast<uint32_t>(type);
        uint8_t* type_ptr = reinterpret_cast<uint8_t*>(&cmd_type);
        bytes.insert(bytes.end(), type_ptr, type_ptr + sizeof(uint32_t));

        uint32_t payload_size = static_cast<uint32_t>(payload.size());
        uint8_t* size_ptr = reinterpret_cast<uint8_t*>(&payload_size);
        bytes.insert(bytes.end(), size_ptr, size_ptr + sizeof(uint32_t));

        bytes.insert(bytes.end(), payload.begin(), payload.end());

        return bytes;
    }

    static DebugCommand deserialize(const std::vector<uint8_t>& bytes) {
        if (bytes.size() < sizeof(uint32_t) * 2) {
            throw std::runtime_error("Insufficient bytes for deserialization.");
        }

        uint32_t cmd_type;
        std::memcpy(&cmd_type, bytes.data(), sizeof(uint32_t));

        uint32_t payload_size;
        std::memcpy(&payload_size, bytes.data() + sizeof(uint32_t), sizeof(uint32_t));

        if (bytes.size() < sizeof(uint32_t) * 2 + payload_size) {
            throw std::runtime_error("Insufficient bytes for payload.");
        }

        std::string payload_data(bytes.begin() + sizeof(uint32_t) * 2,
                                 bytes.begin() + sizeof(uint32_t) * 2 + payload_size);

        return DebugCommand(static_cast<CommandType>(cmd_type), payload_data);
    }
};
