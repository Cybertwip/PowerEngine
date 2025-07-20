#include "MeshPicker.hpp"

#include "filesystem/CompressedSerialization.hpp"
#include "filesystem/DirectoryNode.hpp"

#include "ui/FileView.hpp"

#include <nanogui/layout.h>
#include <nanogui/button.h>
#include <nanogui/imageview.h>
#include <nanogui/label.h>
#include <nanogui/screen.h>
#include <nanogui/vscrollpanel.h>

#include <algorithm>
#include <iostream>
#include <chrono> // For double-click detection

MeshPicker::MeshPicker(nanogui::Screen& parent, std::shared_ptr<DirectoryNode> root_directory_node,
					   std::function<void(std::shared_ptr<DirectoryNode>)> on_model_selected)
: nanogui::Window(parent),
root_directory_node_(root_directory_node),
on_model_selected_(on_model_selected)
{
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
	
	const std::set<std::string>& allowed_extensions = {".psk"};
	
	mFileView = std::make_shared<FileView>(*this, root_directory_node_, true, nullptr, on_model_selected_, "", allowed_extensions);
}

void MeshPicker::refresh_file_list() {
//	mFileView->refresh(mFilterValue);
}

void MeshPicker::ProcessEvents() {
	mFileView->ProcessEvents();
}
