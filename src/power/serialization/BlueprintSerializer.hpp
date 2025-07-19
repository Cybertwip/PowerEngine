#pragma once

#include <string>

// Forward declarations
class NodeProcessor;

/**
 * @class BlueprintSerializer
 * @brief Handles serialization and deserialization of blueprint graphs.
 * This class reads from and writes to standalone blueprint asset files.
 */
class BlueprintSerializer {
public:
    BlueprintSerializer() = default;

    /**
     * @brief Serializes the state of a NodeProcessor to a file.
     * @param node_processor The NodeProcessor instance to serialize.
     * @param filepath The path to the output file.
     */
    void serialize(const NodeProcessor& node_processor, const std::string& filepath);

    /**
     * @brief Deserializes from a file into an existing NodeProcessor.
     * Clears the previous state of the NodeProcessor before loading.
     * @param node_processor The NodeProcessor instance to populate.
     * @param filepath The path to the input blueprint file.
     */
    void deserialize(NodeProcessor& node_processor, const std::string& filepath);
};