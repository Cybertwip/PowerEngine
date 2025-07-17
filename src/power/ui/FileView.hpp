#pragma once

#include <nanogui/widget.h>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>

// Forward declarations
namespace nanogui {
class Button;
class Label;
class VScrollPanel;
}
class DirectoryNode;

class FileView : public nanogui::Widget {
public:
	FileView(
			 nanogui::Widget& parent,
			 DirectoryNode& root_directory_node,
			 bool recursive,
			 std::function<void(std::shared_ptr<DirectoryNode>)> onFileClicked,
			 std::function<void(std::shared_ptr<DirectoryNode>)> onFileSelected,
			 const std::string& filter_text = "",
			 const std::set<std::string>& allowed_extensions = {".png", ".fbx"}
			 );
	
	~FileView();
	
	void refresh();
	void set_filter_text(const std::string& filter);
	void ProcessEvents();
	
private:
	void populate_file_view();
	void create_file_item(const std::shared_ptr<DirectoryNode>& node);
	int get_icon_for_file(const DirectoryNode& node) const;
	void collect_nodes_recursive(DirectoryNode* node, std::vector<std::shared_ptr<DirectoryNode>>& collected_nodes);
	
	// Navigation and interaction handlers
	void handle_file_interaction(DirectoryNode& node);
	void handle_file_click(DirectoryNode& node);
	
	// Drag and Drop functionality
	void initiate_drag_operation(const std::shared_ptr<DirectoryNode>& node);
	
	// MODIFIED: Keep track of the initial root and the currently viewed directory.
	DirectoryNode& m_initial_root_node;
	DirectoryNode& m_current_directory_node;
	
	// Callbacks
	std::function<void(std::shared_ptr<DirectoryNode>)> mOnFileClicked;
	std::function<void(std::shared_ptr<DirectoryNode>)> mOnFileSelected;
	
	// UI components as shared pointers
	std::shared_ptr<nanogui::VScrollPanel> m_vscroll;
	std::shared_ptr<nanogui::Widget> m_content_panel;
	std::shared_ptr<nanogui::Label> m_drag_payload;
	
	// State variables
	std::string m_filter_text;
	std::set<std::string> m_allowed_extensions;
	bool m_recursive;
	bool m_is_dragging = false;
	
	// Selection management
	std::shared_ptr<nanogui::Button> m_selected_button = nullptr;
	std::shared_ptr<DirectoryNode> m_selected_node = nullptr;
	
	// Colors
	nanogui::Color m_normal_button_color;
	nanogui::Color m_selected_button_color;
};
