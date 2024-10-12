#pragma once

#include "filesystem/DirectoryNode.hpp"
#include "filesystem/CompressedSerialization.hpp"
#include <nanogui/nanogui.h>

#include <algorithm>
#include <chrono>
#include <deque>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <string>
#include <vector>

class FileView : public nanogui::Widget {
public:
	FileView(nanogui::Widget& parent,
			 nanogui::Screen& screen,
			 DirectoryNode& root_directory_node,
			 const std::string& filter_text = "",
			 const std::set<std::string>& allowed_extensions = {".psk", ".pma", ".pan", ".psq", ".psn"});
	
	virtual ~FileView();
	
	// Refresh the file view with an optional filter
	void refresh(const std::string& filter_text = "");
	
	// Set the selected directory path
	void set_selected_directory_path(const std::string& path);
	
	// Set the filter text
	void set_filter_text(const std::string& filter);
	
protected:
	// Handle scrolling events
	virtual bool scroll_event(const nanogui::Vector2i& p, const nanogui::Vector2f& rel) override;
	
	// Override draw to implement clipping
	virtual void draw(NVGcontext* ctx) override;
	
	void ProcessEvents();
	
private:
	// Initialize the texture cache
	void initialize_texture_cache();
	
	// Acquire a texture from the cache
	std::shared_ptr<nanogui::Texture> acquire_texture();
	
	// Release a texture back to the cache
	void release_texture(std::shared_ptr<nanogui::Texture> texture);
	
	// Populate the file view with items
	void populate_file_view();
	
	int get_icon_for_file(const DirectoryNode& node) const;
	
	// Find a node by its path
	DirectoryNode* find_node_by_path(DirectoryNode& root, const std::string& path) const;
	
	// Handle file interactions (e.g., double-click)
	void handle_file_interaction(DirectoryNode& node);
	
	// Load thumbnail asynchronously
	void load_thumbnail(const std::shared_ptr<DirectoryNode>& node,
						const std::shared_ptr<nanogui::ImageView>& image_view,
						const std::shared_ptr<nanogui::Texture>& texture);
	
	// Load image data from a file
	std::vector<uint8_t> load_image_data(const std::string& path);
	
	// Wrap textures when scrolling
	void wrap_texture_cache(bool scroll_down);
	
	// Clear all file buttons and related widgets
	void clear_file_buttons();

	float m_accumulated_scroll_delta = 0.0f;
	const float m_scroll_threshold = 1.0f * 144.0f; // Assuming row_height = 144
	
	// Helper methods
	void update_visible_items(int first_visible_row);
	void reuse_textures();
	DirectoryNode* get_node_by_index(int index) const;

	// Members
	DirectoryNode& m_root_directory_node;
	std::string m_selected_directory_path;
	std::string m_filter_text;
	std::set<std::string> m_allowed_extensions;
	
	nanogui::Button* m_selected_button;
	DirectoryNode* m_selected_node;
	
	nanogui::Color m_normal_button_color;
	nanogui::Color m_selected_button_color;
	
	std::mutex m_mutex;
	
	// Texture cache
	std::deque<std::shared_ptr<nanogui::Texture>> m_texture_cache;
	std::deque<std::shared_ptr<nanogui::Texture>> m_used_textures;
	
	// UI Elements
	std::vector<std::shared_ptr<nanogui::Widget>> m_item_containers;
	std::vector<std::shared_ptr<nanogui::Button>> m_file_buttons;
	std::vector<std::shared_ptr<nanogui::ImageView>> m_image_views;
	std::vector<std::shared_ptr<nanogui::Label>> m_name_labels;
	
	// Content widget for manual scrolling
	std::shared_ptr<nanogui::Widget> m_content;

	// Scroll management
	float m_scroll_offset;
	int m_total_rows;
	int m_visible_rows;
	int m_row_height;
	const int m_total_textures = 40;
	
	// Queue to hold thumbnail loading tasks
	std::queue<std::function<void()>> m_thumbnail_load_queue;
	
	// Mutex to protect access to the queue
	std::mutex m_queue_mutex;
	
	// Flag to indicate if a thumbnail is currently being loaded
	bool m_is_loading_thumbnail = false;

};
