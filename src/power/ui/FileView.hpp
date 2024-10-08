#pragma once

#include <nanogui/nanogui.h>

#include <chrono>
#include <memory>
#include <set>
#include <string>
#include <vector>

// Forward declarations
struct DirectoryNode;
class CompressedSerialization;

class FileView : public nanogui::Widget {
public:
	FileView(nanogui::Widget& parent,
			 nanogui::Screen& screen,
			 DirectoryNode& root_directory_node,
			 const std::string& filter_text = "",
			 const std::set<std::string>& allowed_extensions = {".psk", ".pma", ".pan", ".psq", ".psn"});
	
	// Refresh the file view to display the latest contents
	void refresh(const std::string& filter_text = "");
	
	// Setters for dynamic properties
	void set_selected_directory_path(const std::string& path);
	void set_filter_text(const std::string& filter);
	
private:
	// Internal helper functions
	void clear_file_buttons();
	void populate_file_view();
	int get_icon_for_file(const DirectoryNode& node) const;
	DirectoryNode* find_node_by_path(DirectoryNode& root, const std::string& path) const;
	void handle_file_interaction(DirectoryNode& node);
	
	// Member variables
	DirectoryNode& m_root_directory_node;
	std::string m_selected_directory_path;
	std::string m_filter_text;
	std::set<std::string> m_allowed_extensions;
	
	// Containers to hold shared_ptr references
	std::vector<std::shared_ptr<nanogui::Button>> m_file_buttons;
	std::vector<std::shared_ptr<nanogui::Widget>> m_item_containers;
	std::vector<std::shared_ptr<nanogui::ImageView>> m_image_views;
	std::vector<std::shared_ptr<nanogui::Label>> m_name_labels;
	
	std::shared_ptr<nanogui::Button> m_selected_button;
	std::shared_ptr<DirectoryNode> m_selected_node;
	
	nanogui::Color m_normal_button_color;
	nanogui::Color m_selected_button_color;
		
	// Disable copy and assign
	FileView(const FileView&) = delete;
	FileView& operator=(const FileView&) = delete;
};

