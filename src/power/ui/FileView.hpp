#pragma once

#include <nanogui/widget.h>
#include <nanogui/button.h>
#include <nanogui/imageview.h>
#include <nanogui/label.h>
#include <nanogui/texture.h>
#include <memory>
#include <vector>
#include <deque>
#include <set>
#include <string>
#include <mutex>
#include <queue>

// Forward declaration of DirectoryNode
struct DirectoryNode;

class FileView : public nanogui::Widget {
public:
	// Constructor
	FileView(nanogui::Widget& parent,
			 nanogui::Screen& screen,
			 DirectoryNode& root_directory_node,
			 const std::string& filter_text = "",
			 const std::set<std::string>& allowed_extensions = {".psk", ".pma", ".pan", ".psq", ".psn"});
	
	// Destructor
	virtual ~FileView();
	
	// Refresh the view with an optional filter text
	void refresh(const std::string& filter_text = "");
	
	// Handle scroll events
	virtual bool scroll_event(const nanogui::Vector2i& p, const nanogui::Vector2f& rel) override;
	
	// Process pending thumbnail loading tasks
	void ProcessEvents();

protected:
	// Drawing method
	virtual void draw(NVGcontext* ctx) override;
	
private:
	// Initialize texture cache
	void initialize_texture_cache();
	
	// Acquire and release textures
	std::shared_ptr<nanogui::Texture> acquire_texture();
	void release_texture(std::shared_ptr<nanogui::Texture> texture);
	
	// Acquire and release buttons
	std::shared_ptr<nanogui::Button> acquire_button(const std::shared_ptr<DirectoryNode>& child);
		
	// Setters
	void set_selected_directory_path(const std::string& path);
	void set_filter_text(const std::string& filter);
	
	// Clear all file buttons
	void clear_file_buttons();
	
	// Get icon based on file type
	int get_icon_for_file(const DirectoryNode& node) const;
	
	// Find a node by its path
	DirectoryNode* find_node_by_path(DirectoryNode& root, const std::string& path) const;
	
	// Handle interactions like double-click
	void handle_file_interaction(DirectoryNode& node);
	
	// Asynchronously load thumbnail
	void load_thumbnail(const std::shared_ptr<DirectoryNode>& node,
						const std::shared_ptr<nanogui::ImageView>& image_view,
						const std::shared_ptr<nanogui::Texture>& texture);
	
	// Load image data from a file
	std::vector<uint8_t> load_image_data(const std::string& path);
	
	// Populate the file view with current directory contents
	void populate_file_view();
	
	// Load thumbnails for a specific row
	void load_row_thumbnails(int row_index);
	
	// Get DirectoryNode by index
	DirectoryNode* get_node_by_index(int index) const;
	
	// Update visible items based on scroll direction
	void update_visible_items(int first_visible_row, int direction);
	
	// Add or update a widget for a specific index and node
	void add_or_update_widget(int index, const std::shared_ptr<DirectoryNode>& child);
	
	// Remove widgets that are no longer visible
	void remove_invisible_widgets(int first_visible_row);
	
	// Helper to determine if a widget is visible
	bool is_widget_visible(int index) const;
	
	// Member Variables
	nanogui::Screen& m_screen;
	DirectoryNode& m_root_directory_node;
	std::string m_selected_directory_path;
	std::string m_filter_text;
	std::set<std::string> m_allowed_extensions;
	
	std::shared_ptr<nanogui::Button> m_selected_button;
	std::shared_ptr<DirectoryNode> m_selected_node;
	
	nanogui::Color m_normal_button_color;
	nanogui::Color m_selected_button_color;
	
	float m_scroll_offset;
	float m_accumulated_scroll_delta;
	int m_total_rows;
	int m_visible_rows;
	int m_row_height;
	int m_total_textures;
	int m_previous_first_visible_row;
	bool m_is_loading_thumbnail;
	
	int m_previous_scroll_offset;
	
	// Caching Mechanisms
	std::deque<std::shared_ptr<nanogui::Texture>> m_texture_cache;
	std::deque<std::shared_ptr<nanogui::Texture>> m_used_textures;
	
	// Widgets
	std::shared_ptr<nanogui::Widget> m_content;
	std::vector<std::shared_ptr<nanogui::Button>> m_file_buttons;
	std::vector<std::shared_ptr<nanogui::ImageView>> m_image_views;
	std::vector<std::shared_ptr<nanogui::Label>> m_name_labels;
	std::vector<std::shared_ptr<nanogui::Widget>> m_item_containers;
	std::vector<DirectoryNode*> m_nodes;
	std::vector<std::shared_ptr<DirectoryNode>> m_all_nodes;
	
	// Synchronization
	std::mutex m_mutex;
	std::mutex m_queue_mutex;
	
	// Thumbnail Loading Queue
	std::queue<std::function<void()>> m_thumbnail_load_queue;
};
