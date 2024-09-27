#include "MeshPicker.hpp"
#include <nanogui/layout.h>
#include <nanogui/button.h>
#include <nanogui/label.h>
#include <algorithm>
#include <iostream>


MeshPicker::MeshPicker(nanogui::Widget* parent, DirectoryNode& root_directory_node,
					   std::function<void(const std::string&)> on_model_selected)
: nanogui::Window(parent->screen()),
root_directory_node_(root_directory_node),
on_model_selected_(on_model_selected)
{
	setup_ui();
	search_model_files(root_directory_node_);
	refresh_file_list();
}

void MeshPicker::setup_ui() {
	// Layout
	set_layout(new nanogui::BoxLayout(nanogui::Orientation::Vertical,
									  nanogui::Alignment::Fill, 0, 5));
	
	// Filter Box
	filter_box_ = new nanogui::TextBox(this, "");
	filter_box_->set_placeholder("Filter by name...");
	filter_box_->set_fixed_height(25);
	filter_box_->set_callback([this](const std::string& value) {
		refresh_file_list();
		return true;
	});
	
	// Scrollable File List
	file_list_widget_ = new nanogui::Widget(this);
	file_list_widget_->set_layout(new nanogui::BoxLayout(nanogui::Orientation::Vertical,
														 nanogui::Alignment::Fill, 0, 5));
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
	// Clear existing items
	for (auto child : file_list_widget_->children()) {
		file_list_widget_->remove_child(child);
	}
	
	std::string filter = filter_box_->value();
	std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);
	
	for (const auto& model_path : model_files_) {
		std::string filename = model_path.substr(model_path.find_last_of("/\\") + 1);
		std::string filename_lower = filename;
		std::transform(filename_lower.begin(), filename_lower.end(), filename_lower.begin(), ::tolower);
		
		if (!filter.empty() && filename_lower.find(filter) == std::string::npos) {
			continue; // Skip files that don't match the filter
		}
		
		// Create a button for each model file
		auto model_button = new nanogui::Button(file_list_widget_, filename, FA_FILE);
		model_button->set_flags(nanogui::Button::Flags::RadioButton);
		model_button->set_fixed_height(30);
		model_button->set_background_color(nanogui::Color(0.7f, 0.7f, 0.7f, 1.0f));
		model_button->set_font_size(14);
		
		// Capture double-click
		model_button->set_callback([this, model_path](){
			static std::chrono::time_point<std::chrono::high_resolution_clock> last_click_time =
			std::chrono::high_resolution_clock::now() - std::chrono::milliseconds(500);
			auto current_time = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_click_time).count();
			
			if (duration < 400) { // Double-click detected
				handle_double_click(model_path);
				last_click_time = current_time - std::chrono::milliseconds(500); // Reset
				return true;
			}
			last_click_time = current_time;
			return false;
		});
	}
	
	perform_layout(screen()->nvg_context());
}

void MeshPicker::handle_double_click(const std::string& model_path) {
	if (on_model_selected_) {
		on_model_selected_(model_path);
	}
}
