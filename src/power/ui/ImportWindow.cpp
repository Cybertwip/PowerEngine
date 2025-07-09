#include "ImportWindow.hpp"

#include "ShaderManager.hpp"

#include "ui/ResourcesPanel.hpp"

#include <nanogui/button.h>
#include <nanogui/checkbox.h>
#include <nanogui/layout.h>
#include <nanogui/screen.h>

#include <openssl/md5.h>

#include <sstream>

ImportWindow::ImportWindow(nanogui::Screen& parent, ResourcesPanel& resourcesPanel) : nanogui::Window(parent), mResourcesPanel(resourcesPanel) {
	
	set_fixed_size(nanogui::Vector2i(400, 120)); // Adjusted size after removing preview
	set_layout(std::make_unique<nanogui::GroupLayout>());
	set_title("Import Asset");
	
	// Close Button
	mCloseButton = std::make_shared<nanogui::Button>(button_panel(), "X");
	mCloseButton->set_fixed_size(nanogui::Vector2i(20, 20));
	mCloseButton->set_callback([this]() {
		this->set_visible(false);
		this->set_modal(false);
	});
	
	// Create "Import" button
	mImportButton = std::make_shared<nanogui::Button>(*this, "Import");
	mImportButton->set_callback([this]() {
		nanogui::async([this](){this->ImportIntoProject();});
	});
	mImportButton->set_tooltip("Import the selected asset");
	
	mImportButton->set_fixed_width(256);
}

void ImportWindow::Preview(const std::string& path, const std::string& directory) {
	set_visible(true);
	set_modal(true);
	
	// Mesh preview logic has been removed.
	// This function now just shows the window.
}

void ImportWindow::ImportIntoProject() {
	// All mesh and animation processing and persistence logic has been removed.
	
	// Asynchronously refresh the resources panel
	nanogui::async([this](){
		mResourcesPanel.refresh_file_view();
	});
	
	// Close the import window
	set_visible(false);
	set_modal(false);
}

void ImportWindow::ProcessEvents() {
	// Event processing for mesh preview canvas has been removed.
}
