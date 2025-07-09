#pragma once

#include <nanogui/window.h>

#include <memory>
#include <string>

// Forward declarations
class ResourcesPanel;

namespace nanogui {
class Button;
class Widget;
class Screen;
}

class ImportWindow : public nanogui::Window {
public:
	// Constructor updated to remove dependencies on mesh-related managers
	ImportWindow(nanogui::Screen& parent, ResourcesPanel& resourcesPanel);
	
	// Displays the window to the user
	void Preview(const std::string& path, const std::string& directory);
	
	// Processes UI events
	void ProcessEvents();
	
private:
	// Handles the import logic after the user confirms
	void ImportIntoProject();
	
	// A reference to the panel that displays project resources
	ResourcesPanel& mResourcesPanel;
	
	// UI Elements
	std::shared_ptr<nanogui::Button> mCloseButton;
	std::shared_ptr<nanogui::Button> mImportButton;
};
