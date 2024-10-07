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

MeshPicker::MeshPicker(std::shared_ptr<Widget> parent, DirectoryNode& root_directory_node,
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
	set_fixed_size(nanogui::Vector2i(400, 320));
	set_layout(std::make_shared<nanogui::GroupLayout>());
	
	set_title("Select Mesh");
	
	// Close Button
	mCloseButton = std::make_shared<nanogui::Button>(button_panel(), "X");
	mCloseButton->set_fixed_size(nanogui::Vector2i(20, 20));
	mCloseButton->set_callback([this]() {
		set_visible(false);
		set_modal(false);
	});
	
	// Filter Box
	filter_box_ = std::make_shared<nanogui::TextBox>(shared_from_this(), "");
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
	mScrollPanel = std::make_shared<nanogui::VScrollPanel>(shared_from_this());
	mScrollPanel->set_fixed_size(nanogui::Vector2i(380, 270)); // Adjust size as needed

	// Set layout for the scroll panel
	file_list_widget_ = std::make_shared<nanogui::Widget>(mScrollPanel);
	file_list_widget_->set_layout(std::make_shared<nanogui::BoxLayout>(nanogui::Orientation::Horizontal, nanogui::Alignment::Minimum));
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
	// It's safer to remove all children from file_list_widget_
	auto fileview_children = file_list_widget_->children();
	
	for (auto& file_view_child : fileview_children) {
		auto child_children = file_view_child->children();
		for (auto& child_child : child_children) {
			file_view_child->remove_child(child_child);
		}
		file_list_widget_->remove_child(file_view_child);
	}

	std::transform(mFilterValue.begin(), mFilterValue.end(), mFilterValue.begin(), ::tolower);
	
	for (const auto& model_path : model_files_) {
		std::string filename = model_path.substr(model_path.find_last_of("/\\") + 1);
		std::string filename_lower = filename;
		std::transform(filename_lower.begin(), filename_lower.end(), filename_lower.begin(), ::tolower);
		
		if (!mFilterValue.empty() && filename_lower.find(mFilterValue) == std::string::npos) {
			continue; // Skip files that don't match the filter
		}
		
		// Create a button for each model file
		std::shared_ptr<Widget> itemContainer = std::make_shared<nanogui::Widget>(file_list_widget_);
		itemContainer->set_layout(std::make_shared<nanogui::BoxLayout>(
														 nanogui::Orientation::Vertical, nanogui::Alignment::Middle, 0, 5));
		
		auto icon = new nanogui::Button(itemContainer, "");
		
		icon->set_fixed_size(nanogui::Vector2i(128, 128));
		
		// deserialize thumbnail here
		CompressedSerialization::Deserializer deserializer;
		
		deserializer.load_from_file(filename);
		
		std::vector<uint8_t> pixels;
		pixels.resize(512 * 512 * 4);
		
		uint64_t thumbnail_size = 0;
		
		
		uint64_t hash_id[] = { 0, 0 };
		
		deserializer.read_header_raw(hash_id, sizeof(hash_id)); // To increase the offset and read the thumbnail size
		
		deserializer.read_header_uint64(thumbnail_size);
		
		if (thumbnail_size != 0) {
			deserializer.read_header_raw(pixels.data(), thumbnail_size);
			
			auto imageView = std::make_shared<nanogui::ImageView>(icon);
			imageView->set_size(icon->fixed_size());
			
			imageView->set_fixed_size(icon->fixed_size());
			
			imageView->set_image(std::make_shared<nanogui::Texture>(
													  pixels.data(),
													  pixels.size(),
								 512, 512,			  nanogui::Texture::InterpolationMode::Nearest,
													  nanogui::Texture::InterpolationMode::Nearest,
													  nanogui::Texture::WrapMode::ClampToEdge));
			
			imageView->image()->resize(nanogui::Vector2i(256, 256));
			
			
			imageView->set_visible(true);
		}
		
		auto name = new nanogui::Label(itemContainer, filename);
		name->set_fixed_width(120);

		// Capture double-click
		icon->set_callback([this, model_path](){
			static std::chrono::time_point<std::chrono::high_resolution_clock> last_click_time =
			std::chrono::high_resolution_clock::now() - std::chrono::milliseconds(500);
			auto current_time = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_click_time).count();
			
			if (duration < 400) { // Double-click detected
				handle_double_click(model_path);
				last_click_time = current_time - std::chrono::milliseconds(500); // Reset
			}
			last_click_time = current_time;
		});
	}
	
	perform_layout(screen()->nvg_context());
}

void MeshPicker::handle_double_click(const std::string& model_path) {
	nanogui::async([this, model_path](){
		if (on_model_selected_) {
			on_model_selected_(model_path);
		}
	});
}
