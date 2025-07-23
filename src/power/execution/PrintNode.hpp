#pragma once

#include "BlueprintNode.hpp"

#include <nanogui/textbox.h>

#include <memory>
#include <string>

class BlueprintCanvas;

class PrintCoreNode : public CoreNode {
public:
    explicit PrintCoreNode(UUID id);

    /**
     * @brief Executes the node's logic: printing a string to the console.
     * @return True to allow the execution flow to continue.
     */
    bool evaluate() override;

private:
    friend class PrintVisualNode; // Allow visual node to access private members

    CorePin& mStringInput;
    nanogui::TextBox* mTextBox = nullptr; // A non-owning pointer to the UI element
};

class PrintVisualNode : public VisualBlueprintNode {
public:
    PrintVisualNode(BlueprintCanvas& parent, nanogui::Vector2i position, nanogui::Vector2i size, PrintCoreNode& coreNode);
};