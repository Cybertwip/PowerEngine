#include "MeshPicker.hpp"

#include "filesystem/CompressedSerialization.hpp"
#include "filesystem/DirectoryNode.hpp"

#include <nanogui/layout.h>
#include <nanogui/button.h>
#include <nanogui/imageview.h>
#include <nanogui/label.h>
#include <nanogui/screen.h>
#include <nanogui/vscrollpanel.h>

#include <algorithm>
#include <iostream>
#include <chrono> // For double-click detection

MeshPicker::MeshPicker(nanogui::Screen& parent, nanogui::Screen& screen, DirectoryNode& root_directory_node,
					   std::function<void(const std::string&)> on_model_selected)
: nanogui::Window(parent, screen),
root_directory_node_(root_directory_node),
on_model_selected_(on_model_selected)
{
	setup_ui();
	search_model_files(root_directory_node_);
	refresh_file_list();

}

void MeshPicker::setup_ui() {
	set_fixed_size(nanogui::Vector2i(400, 320));
	set_layout(std::make_unique<nanogui::GroupLayout>());
	
	set_title("Select Mesh");
	
	// Close Button
	mCloseButton = std::make_shared<nanogui::Button>(button_panel(), "X");
	mCloseButton->set_fixed_size(nanogui::Vector2i(20, 20));
	mCloseButton->set_callback([this]() {
		set_visible(false);
		set_modal(false);
	});
	
	// Filter Box
	filter_box_ = std::make_shared<nanogui::TextBox>(*this, "");
	filter_box_->set_placeholder("Filter by name...");
	filter_box_->set_fixed_height(25);
	filter_box_->set_alignment(nanogui::TextBox::Alignment::Left);
	
	filter_box_->set_editable(true);
	
	filter_box_->set_callback([this](const std::string& value) {
		mFilterValue = value;
		
		nanogui::async([this](){
			refresh_file_list();
		});
		
		return true;
	});
		
	// Scrollable File List
	// Create a ScrollPanel to make the file list scrollable
	mScrollPanel = std::make_shared<nanogui::VScrollPanel>(*this);
	mScrollPanel->set_fixed_size(nanogui::Vector2i(380, 270)); // Adjust size as needed

	// Set layout for the scroll panel
	file_list_widget_ = std::make_shared<nanogui::Widget>(mScrollPanel);
	file_list_widget_->set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Horizontal, nanogui::Alignment::Minimum));
}

void MeshPicker::search_model_files(const DirectoryNode& node) {
	if (!node.IsDirectory) {
		// Check for .pma or .psk extensions
		if (node.FileName.size() >= 4) {
			std::string ext = node.FileName.substr(node.FileName.size() - 4);
			std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
			if (ext == ".pma" || ext == ".psk") {
				model_files_.push_back(node.FullPath);
			}
		}
		return;
	}
	
	for (const auto& child : node.Children) {
		search_model_files(*child);
	}
}

void MeshPicker::refresh_file_list() {
	
	perform_layout(screen().nvg_context());
}

void MeshPicker::handle_double_click(const std::string& model_path) {
	nanogui::async([this, model_path](){
		if (on_model_selected_) {
			on_model_selected_(model_path);
		}
	});
}
