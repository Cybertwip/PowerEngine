// FileView.hpp

#pragma once

#include <nanogui/widget.h>
#include <nanogui/button.h>
#include <nanogui/label.h>
#include <nanogui/imageview.h>
#include <nanogui/texture.h>

#include <memory>
#include <vector>
#include <deque>
#include <mutex>
#include <queue>
#include <functional>
#include <string>
#include <set>

#include "filesystem/DirectoryNode.hpp"

class FileView : public nanogui::Widget {
public:
	FileView(nanogui::Widget& parent,
			 nanogui::Screen& screen,
			 DirectoryNode& root_directory_node,
			 const std::string& filter_text = "",
			 const std::set<std::string>& allowed_extensions = {".psk", ".pma", ".pan", ".psq", ".psn"});
	
	virtual ~FileView();
	
	void refresh(const std::string& filter_text = "");
	void set_selected_directory_path(const std::string& path);
	void set_filter_text(const std::string& filter);
	
	virtual void draw(NVGcontext* ctx) override;
	virtual bool scroll_event(const nanogui::Vector2i& p, const nanogui::Vector2f& rel) override;
	
	void ProcessEvents();
	
protected:
	void initialize_texture_cache();
	std::shared_ptr<nanogui::Texture> acquire_texture();
	void release_texture(std::shared_ptr<nanogui::Texture> texture);
	
	std::shared_ptr<nanogui::Button> acquire_button(const DirectoryNode& child,
													const std::shared_ptr<nanogui::Widget>& item_container,
													const std::shared_ptr<nanogui::Texture>& texture,
													const std::shared_ptr<nanogui::ImageView>& image_view);
	
	void release_button(std::shared_ptr<nanogui::Button> button);

	void clear_file_buttons();
	void populate_file_view();
	DirectoryNode* find_node_by_path(DirectoryNode& root, const std::string& path) const;
	void handle_file_interaction(DirectoryNode& node);
	void load_thumbnail(const std::shared_ptr<DirectoryNode>& node,
						const std::shared_ptr<nanogui::ImageView>& image_view,
						const std::shared_ptr<nanogui::Texture>& texture);
	std::vector<uint8_t> load_image_data(const std::string& path);
	int get_icon_for_file(const DirectoryNode& node) const;
	
	void update_visible_items(int first_visible_row);
	
	void load_row_thumbnails(int row_index); // New method
	DirectoryNode* get_node_by_index(int index) const;
	
private:
	nanogui::Screen& m_screen;
	DirectoryNode& m_root_directory_node;
	std::string m_selected_directory_path;
	std::string m_filter_text;
	std::set<std::string> m_allowed_extensions;
	
	std::shared_ptr<nanogui::Widget> m_content;
	
	std::vector<std::shared_ptr<nanogui::Widget>> m_item_containers;
	std::vector<std::shared_ptr<nanogui::Button>> m_file_buttons;
	std::vector<std::shared_ptr<nanogui::ImageView>> m_image_views;
	std::vector<std::shared_ptr<nanogui::Label>> m_name_labels;
	
	std::vector<DirectoryNode*> m_nodes; // Added to store nodes by index
	
	std::shared_ptr<nanogui::Button> m_selected_button;
	std::shared_ptr<DirectoryNode> m_selected_node;
	
	nanogui::Color m_normal_button_color;
	nanogui::Color m_selected_button_color;
	
	std::deque<std::shared_ptr<nanogui::Texture>> m_texture_cache;
	std::deque<std::shared_ptr<nanogui::Texture>> m_used_textures;
	
	int m_total_textures;
	
	std::deque<std::shared_ptr<nanogui::Button>> m_button_cache;

	
	float m_scroll_offset;
	float m_accumulated_scroll_delta;
	const float m_scroll_threshold = 10.0f; // Adjust as needed
	
	int m_total_rows;
	int m_visible_rows;
	int m_row_height;
	
	int m_previous_first_visible_row; // Added to keep track of previous first visible row
	
	std::mutex m_mutex;
	std::mutex m_queue_mutex;
	
	std::queue<std::function<void()>> m_thumbnail_load_queue;
	bool m_is_loading_thumbnail;
	
	std::vector<std::shared_ptr<DirectoryNode>> m_all_nodes;
};

