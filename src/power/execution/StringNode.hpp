#pragma once

#include "BlueprintNode.hpp"

#include <nanogui/textbox.h>

#include <memory>
#include <string>

class BlueprintCanvas;

class StringCoreNode : public DataCoreNode {
public:
    explicit StringCoreNode(UUID id);

    /**
     * @brief Sets the node's internal string value.
     */
    void set_data(std::optional<std::variant<Entity, std::string, long, float, bool>> data) override;

    /**
     * @brief Gets the node's internal string value.
     */
    std::optional<std::variant<Entity, std::string, long, float, bool>> get_data() const override;
    
private:
    std::string mValue;
};

class StringVisualNode : public VisualBlueprintNode {
public:
    StringVisualNode(BlueprintCanvas& parent, nanogui::Vector2i position, nanogui::Vector2i size, StringCoreNode& coreNode);
    
private:
    nanogui::TextBox& mTextBox;
    StringCoreNode& mCoreNode;
};
