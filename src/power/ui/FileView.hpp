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
	// MODIFIED: Constructor now accepts a shared_ptr for the root node to ensure safe memory management.
	FileView(
			 nanogui::Widget& parent,
			 std::shared_ptr<DirectoryNode> root_directory_node,
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
	nanogui::Vector2i preferred_size(NVGcontext* ctx) override;

	void perform_layout(NVGcontext* ctx) override;

	void populate_file_view();
	void create_file_item(const std::shared_ptr<DirectoryNode>& node);
	int get_icon_for_file(const DirectoryNode& node) const;
	// MODIFIED: Takes a shared_ptr to align with the rest of the memory management strategy.
	void collect_nodes_recursive(std::shared_ptr<DirectoryNode> node, std::vector<std::shared_ptr<DirectoryNode>>& collected_nodes);
	
	// MODIFIED: Handlers now take a const shared_ptr reference for consistency and safety.
	void handle_file_interaction(const std::shared_ptr<DirectoryNode>& node);
	void handle_file_click(const std::shared_ptr<DirectoryNode>& node);
	
	// Drag and Drop functionality
	void initiate_drag_operation(const std::shared_ptr<DirectoryNode>& node);
	
	// MODIFIED: Use shared_ptr to manage the lifetime of DirectoryNode objects safely, preventing dangling pointers.
	std::shared_ptr<DirectoryNode> m_initial_root_node;
	std::shared_ptr<DirectoryNode> m_current_directory_node;
	
	// Callbacks
	std::function<void(std::shared_ptr<DirectoryNode>)> mOnFileClicked;
	std::function<void(std::shared_ptr<DirectoryNode>)> mOnFileSelected;
	
	// UI components
	std::shared_ptr<nanogui::VScrollPanel> m_vscroll;
	std::shared_ptr<nanogui::Widget> m_content_panel;
	std::shared_ptr<nanogui::Label> m_drag_payload;
	std::shared_ptr<nanogui::Button> m_cd_up_button;
	std::vector<std::shared_ptr<nanogui::Button>> m_file_buttons;
	
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
