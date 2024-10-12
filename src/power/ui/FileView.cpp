// FileView.cpp

#include "FileView.hpp"

#include <nanogui/layout.h>
#include <nanogui/screen.h>

#include <nanovg.h>

#include <future>
#include <iostream>
#include <algorithm>
#include <chrono>

#include "filesystem/CompressedSerialization.hpp" // Assuming this is needed for load_image_data

// Constructor
FileView::FileView(nanogui::Widget& parent,
				   nanogui::Screen& screen,
				   DirectoryNode& root_directory_node,
				   const std::string& filter_text,
				   const std::set<std::string>& allowed_extensions)
: nanogui::Widget(parent, screen),
m_screen(screen),
m_root_directory_node(root_directory_node),
m_selected_directory_path(root_directory_node.FullPath),
m_filter_text(filter_text),
m_allowed_extensions(allowed_extensions),
m_selected_button(nullptr),
m_selected_node(nullptr),
m_normal_button_color(nanogui::Color(200, 200, 200, 255)),
m_selected_button_color(nanogui::Color(100, 100, 255, 255)),
m_scroll_offset(0.0f),
m_accumulated_scroll_delta(0.0f),
m_total_rows(0),
m_visible_rows(3),
m_row_height(144), // Assuming row height of 144 pixels
m_total_textures(40), // Example value, adjust as needed
m_previous_first_visible_row(0),
m_is_loading_thumbnail(false)
{
	// Initialize main layout using GridLayout
	auto grid_layout = std::unique_ptr<nanogui::GridLayout>(new nanogui::GridLayout(
											   nanogui::Orientation::Horizontal,
											   8, // Number of columns
											   nanogui::Alignment::Middle,
											   8, // Margin
											   8  // Spacing between widgets
											   ));
	
	set_layout(std::move(grid_layout));
	
	// Initialize texture cache
	initialize_texture_cache();
	
	// Create a content widget that will hold all file items
	m_content = std::make_shared<nanogui::Widget>(*this, screen);
	m_content->set_layout(std::unique_ptr<nanogui::GridLayout>(new nanogui::GridLayout(
												  nanogui::Orientation::Horizontal,
												  8, // Number of columns
												  nanogui::Alignment::Middle,
												  8, // Margin
												  8  // Spacing
												  )));
	
	m_content->set_position(nanogui::Vector2i(0, 0));
	m_content->set_fixed_size(nanogui::Vector2i(this->width(), 288));
	
	// Initial population
	refresh();
}

// Destructor
FileView::~FileView() {
	// Release all used textures back to the cache
	for (auto& texture : m_used_textures) {
		m_texture_cache.push_back(texture);
	}
	m_used_textures.clear();
}

void FileView::initialize_texture_cache() {
	for (int i = 0; i < m_total_textures; ++i) {
		// Create a placeholder texture (e.g., blank or default icon)
		auto texture = std::make_shared<nanogui::Texture>(
														  nanogui::Texture::PixelFormat::RGBA,       // Set pixel format to RGBA
														  nanogui::Texture::ComponentFormat::UInt8,  // Use unsigned 8-bit components for each channel
														  nanogui::Vector2i(512, 512),
														  nanogui::Texture::InterpolationMode::Bilinear,
														  nanogui::Texture::InterpolationMode::Nearest,
														  nanogui::Texture::WrapMode::ClampToEdge
														  );
		m_texture_cache.push_back(texture);
	}
}

std::shared_ptr<nanogui::Texture> FileView::acquire_texture() {
	if (m_texture_cache.empty()) {
		// Handle texture exhaustion by reusing the oldest texture
		if (!m_used_textures.empty()) {
			auto texture = m_used_textures.front();
			m_used_textures.pop_front();
			return texture;
		}
		// If no textures are used, return nullptr
		return nullptr;
	}
	auto texture = m_texture_cache.front();
	m_texture_cache.pop_front();
	m_used_textures.push_back(texture);
	return texture;
}

void FileView::release_texture(std::shared_ptr<nanogui::Texture> texture) {
	auto it = std::find(m_used_textures.begin(), m_used_textures.end(), texture);
	if (it != m_used_textures.end()) {
		m_used_textures.erase(it);
		m_texture_cache.push_back(texture);
	}
}

std::shared_ptr<nanogui::Button> FileView::acquire_button(const DirectoryNode& child,
														  const std::shared_ptr<nanogui::Widget>& item_container,
														  const std::shared_ptr<nanogui::Texture>& texture,
														  const std::shared_ptr<nanogui::ImageView>& image_view) {
	// Reuse button from cache or create a new one
	std::shared_ptr<nanogui::Button> icon_button;
	if (!m_button_cache.empty()) {
		icon_button = m_button_cache.front();
		m_button_cache.pop_front();
	} else {
		icon_button = std::make_shared<nanogui::Button>(*item_container, screen(), "", get_icon_for_file(child));
	}
	
	// Setup button properties
	icon_button->set_fixed_size(nanogui::Vector2i(144, 144));
	icon_button->set_background_color(m_normal_button_color);
	
	// Set the callback for button interactions
	icon_button->set_callback([this, icon_button, image_view, &child]() {
		int file_icon = get_icon_for_file(child);
		
		// Handle selection
		if (m_selected_button && m_selected_button != icon_button) {
			m_selected_button->set_background_color(m_normal_button_color);
		}
		m_selected_button = icon_button;
		icon_button->set_background_color(m_selected_button_color);
		
		// Store selected node
		m_selected_node = std::make_shared<DirectoryNode>(child);
		
		// Handle double-click
		static std::chrono::time_point<std::chrono::high_resolution_clock> last_click_time = std::chrono::high_resolution_clock::time_point::min();
		auto current_click_time = std::chrono::high_resolution_clock::now();
		
		if (last_click_time != std::chrono::high_resolution_clock::time_point::min()) {
			auto click_duration = std::chrono::duration_cast<std::chrono::milliseconds>(current_click_time - last_click_time).count();
			
			if (click_duration < 400) { // 400 ms threshold for double-click
				nanogui::async([this]() {
					std::lock_guard<std::mutex> lock(m_mutex);
					if (m_selected_node) {
						handle_file_interaction(*m_selected_node);
					}
				});
				
				last_click_time = std::chrono::high_resolution_clock::time_point::min();
			} else {
				last_click_time = current_click_time;
			}
		} else {
			last_click_time = current_click_time;
		}
	});
	
	return icon_button;
}

void FileView::release_button(std::shared_ptr<nanogui::Button> button) {
	m_button_cache.push_back(button);
}

void FileView::set_selected_directory_path(const std::string& path) {
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_selected_directory_path = path;
	}
	refresh(); // Refresh the view with the new directory
}

void FileView::set_filter_text(const std::string& filter) {
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_filter_text = filter;
	}
	refresh(); // Refresh the view with the new filter
}

void FileView::clear_file_buttons() {
	for (auto& button : m_file_buttons) {
		release_button(button);
	}
	m_file_buttons.clear();
}

// Determine the appropriate icon for the file type
int FileView::get_icon_for_file(const DirectoryNode& node) const {
	if (node.IsDirectory) return FA_FOLDER;
	if (node.FileName.find(".psk") != std::string::npos) return FA_WALKING;
	if (node.FileName.find(".pma") != std::string::npos) return FA_OBJECT_GROUP;
	if (node.FileName.find(".pan") != std::string::npos) return FA_PERSON_BOOTH;
	
	// More conditions for other file types...
	return FA_FILE; // Default icon
}

// Recursive function to find a node by its path
DirectoryNode* FileView::find_node_by_path(DirectoryNode& root, const std::string& path) const {
	if (root.FullPath == path) {
		return &root;
	}
	for (const auto& child : root.Children) {
		if (child->FullPath == path) {
			return child.get();
		}
		if (child->IsDirectory) {
			DirectoryNode* found = find_node_by_path(*child, path);
			if (found) {
				return found;
			}
		}
	}
	return nullptr;
}

// Handle file interactions (e.g., double-click)
void FileView::handle_file_interaction(DirectoryNode& node) {
	if (node.IsDirectory) {
		// Change the selected directory path and refresh
		set_selected_directory_path(node.FullPath);
	} else {
		// Handle file opening or other interactions
		std::cout << "Opening file: " << node.FullPath << std::endl;
		// Implement actual file opening logic here
	}
}

// Asynchronously load thumbnail
void FileView::load_thumbnail(const std::shared_ptr<DirectoryNode>& node,
							  const std::shared_ptr<nanogui::ImageView>& image_view,
							  const std::shared_ptr<nanogui::Texture>& texture) {
	nanogui::async([this, node, image_view, texture]() {
		// Load and process the thumbnail
		// Example: Read image data from file
		std::vector<uint8_t> thumbnail_data = load_image_data(node->FullPath);
		
		if (!thumbnail_data.empty()) {
			texture->resize(nanogui::Vector2i(512, 512));
			nanogui::Texture::decompress_into(thumbnail_data, *texture);
			
			image_view->set_image(texture);
			image_view->set_visible(true);
			
			texture->resize(nanogui::Vector2i(288, 288));
			m_content->perform_layout(m_screen.nvg_context());
		}
	});
}

// Load image data from a file
std::vector<uint8_t> FileView::load_image_data(const std::string& path) {
	CompressedSerialization::Deserializer deserializer;
	deserializer.load_from_file(path);
	
	std::vector<uint8_t> data;
	uint64_t thumbnail_size = 0;
	uint64_t hash_id[] = {0, 0};
	
	deserializer.read_header_raw(hash_id, sizeof(hash_id)); // To increase the offset and read the thumbnail size
	deserializer.read_header_uint64(thumbnail_size);
	
	if (thumbnail_size != 0) {
		data.resize(512 * 512 * 4);
		deserializer.read_header_raw(data.data(), thumbnail_size);
	}
	return data;
}

void FileView::draw(NVGcontext* ctx) {
	nvgSave(ctx);
	nvgTranslate(ctx, m_pos.x(), m_pos.y());
	
	// Use intersect scissor to respect any existing scissor regions
	auto intersectionSize = parent()->get().size() - m_pos;
	nvgIntersectScissor(ctx, 0, 0, intersectionSize.x(), intersectionSize.y());
	
	for (auto& view : m_image_views) {
		view->set_scissor_rect(absolute_position(), intersectionSize);
	}
	
	if (m_content->visible())
		m_content->draw(ctx);
	
	nvgRestore(ctx);
}

// Implement load_row_thumbnails
void FileView::load_row_thumbnails(int row_index) {
	// Ensure the row index is within valid range
	if (row_index < 0 || row_index >= m_total_rows) {
		return;
	}
	
	for (int col = 0; col < 8; ++col) { // Assuming 8 columns
		int index = row_index * 8 + col;
		if (index >= m_file_buttons.size()) {
			break;
		}
		
		auto icon_button = m_file_buttons[index];
		auto image_view = m_image_views[index];
		auto name_label = m_name_labels[index];
		
		// Determine the corresponding DirectoryNode
		DirectoryNode* node = get_node_by_index(index);
		if (!node) {
			continue;
		}
		
		// Acquire a texture from the cache
		auto texture = acquire_texture();
		if (!texture) {
			// Handle texture exhaustion by reusing the oldest texture
			if (!m_used_textures.empty()) {
				texture = m_used_textures.front();
				m_used_textures.pop_front();
			} else {
				// Create a new placeholder texture if necessary
				texture = std::make_shared<nanogui::Texture>(
															 nanogui::Texture::PixelFormat::RGBA,
															 nanogui::Texture::ComponentFormat::UInt8,
															 nanogui::Vector2i(512, 512),
															 nanogui::Texture::InterpolationMode::Bilinear,
															 nanogui::Texture::InterpolationMode::Nearest,
															 nanogui::Texture::WrapMode::ClampToEdge
															 );
			}
		}
		
		// Update the ImageView with the new texture
		image_view->set_image(texture);
		image_view->set_visible(true);
		
		// Asynchronously load the new thumbnail
		m_thumbnail_load_queue.push([this, node, image_view, texture]() {
			load_thumbnail(node->shared_from_this(), image_view, texture);
		});
	}
	
	// Optionally perform layout updates
	m_content->perform_layout(m_screen.nvg_context());
}

DirectoryNode* FileView::get_node_by_index(int index) const {
	if (index < 0 || index >= m_nodes.size()) {
		return nullptr;
	}
	return m_nodes[index];
}

void FileView::ProcessEvents() {
	std::function<void()> task;
	
	{
		std::lock_guard<std::mutex> lock(m_queue_mutex);
		if (!m_is_loading_thumbnail && !m_thumbnail_load_queue.empty()) {
			task = m_thumbnail_load_queue.front();
			m_thumbnail_load_queue.pop();
			m_is_loading_thumbnail = true;
		}
	}
	
	if (task) {
		// Execute the task asynchronously
		nanogui::async([this, task]() {
			task(); // Load the thumbnail
			
			// After task completion, reset the loading flag
			std::lock_guard<std::mutex> lock(m_queue_mutex);
			m_is_loading_thumbnail = false;
		});
	}
}


void FileView::refresh(const std::string& filter_text) {
	std::lock_guard<std::mutex> lock(m_mutex);
	if (!filter_text.empty()) {
		m_filter_text = filter_text;
	}
	
	// Clear existing buttons and selection
	clear_file_buttons();
	m_selected_button = nullptr;
	m_selected_node = nullptr;
	
	// Refresh the root directory node to get the latest contents
	m_root_directory_node.refresh(m_allowed_extensions);
	
	// Clear existing child widgets in the content
	m_content->shed_children();
	
	// Clear stored references to previously allocated objects
	m_item_containers.clear();
	m_image_views.clear();
	m_name_labels.clear();
	m_nodes.clear();
	
	// Reset scroll offset and row indices
	m_scroll_offset = 0.0f;
	m_previous_first_visible_row = 0;
	
	// Populate the file view with updated contents
	if (!m_root_directory_node.Children.empty()) {
		populate_file_view();
	}
	
	// Perform layout to apply all changes
	perform_layout(m_screen.nvg_context());
}

void FileView::populate_file_view() {
	if (m_selected_directory_path.empty()) {
		return;
	}
	
	DirectoryNode* selected_node = find_node_by_path(m_root_directory_node, m_selected_directory_path);
	if (!selected_node) {
		return;
	}
	
	selected_node->refresh(m_allowed_extensions);
	
	const int columns = 8;
	int current_index = 0;
	
	m_nodes.clear();
	
	// Filtered list of children
	std::vector<std::shared_ptr<DirectoryNode>> filtered_children;
	
	for (const auto& child : selected_node->Children) {
		if (!m_filter_text.empty()) {
			std::string filename = child->FileName;
			std::string filter = m_filter_text;
			std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
			std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);
			if (filename.find(filter) == std::string::npos) {
				continue;
			}
		}
		filtered_children.push_back(child);
	}
	
	m_total_rows = (filtered_children.size() + columns - 1) / columns;
	
	// Only load the initial visible rows
	int max_items = m_visible_rows * columns;
	
	for (int i = 0; i < std::min((int)filtered_children.size(), max_items); ++i) {
		const auto& child = filtered_children[i];
		
		auto item_container = std::make_shared<nanogui::Widget>(*m_content, m_screen);
		item_container->set_fixed_size(nanogui::Vector2i(144, m_row_height));
		m_item_containers.push_back(item_container);
		
		// Acquire a texture from cache
		auto texture = acquire_texture();
		if (!texture) {
			texture = std::make_shared<nanogui::Texture>(nanogui::Texture::PixelFormat::RGBA,
														 nanogui::Texture::ComponentFormat::UInt8,
														 nanogui::Vector2i(512, 512),
														 nanogui::Texture::InterpolationMode::Bilinear,
														 nanogui::Texture::InterpolationMode::Nearest,
														 nanogui::Texture::WrapMode::ClampToEdge);
		}
		
		// Create the image view
		auto image_view = std::make_shared<nanogui::ImageView>(*item_container, m_screen);
		image_view->set_size(nanogui::Vector2i(144, 144));
		image_view->set_fixed_size(nanogui::Vector2i(144, 144));
		image_view->set_image(texture);
		image_view->set_visible(true);
		m_image_views.push_back(image_view);
		
		// Acquire or reuse the button
		auto icon_button = acquire_button(*child, item_container, texture, image_view);
		m_file_buttons.push_back(icon_button);
		m_nodes.push_back(child.get());
		
		// Async thumbnail loading
		m_thumbnail_load_queue.push([this, child, image_view, texture]() {
			load_thumbnail(child, image_view, texture);
		});
		
		// Label
		auto name_label = std::make_shared<nanogui::Label>(*item_container, m_screen, child->FileName);
		name_label->set_fixed_width(144);
		name_label->set_position(nanogui::Vector2i(0, 110));
		name_label->set_alignment(nanogui::Label::Alignment::Center);
		m_name_labels.push_back(name_label);
	}
	
	m_all_nodes = filtered_children; // Store all filtered nodes for future use
	
	m_content->perform_layout(m_screen.nvg_context());
}

bool FileView::scroll_event(const nanogui::Vector2i& p, const nanogui::Vector2f& rel) {
	std::lock_guard<std::mutex> lock(m_mutex);
	
	float scroll_sensitivity = 30.0f; // Pixels per scroll unit
	float scroll_delta = rel.y() * scroll_sensitivity;
	
	// Update scroll offset
	m_scroll_offset -= scroll_delta;
	
	// Clamp the scroll offset
	int content_height = m_total_rows * m_row_height;
	int view_height = m_visible_rows * m_row_height;
	
	if (content_height > view_height) {
		if (m_scroll_offset < 0.0f) {
			m_scroll_offset = 0.0f;
		}
		if (m_scroll_offset > (content_height - view_height)) {
			m_scroll_offset = content_height - view_height;
		}
	} else {
		m_scroll_offset = 0.0f;
	}
	
	// Calculate the first visible row
	int new_first_visible_row = static_cast<int>(m_scroll_offset / m_row_height);
	
	// Check if we need to update visible items
	if (new_first_visible_row != m_previous_first_visible_row) {
		// Update visible items
		update_visible_items(new_first_visible_row);
		m_previous_first_visible_row = new_first_visible_row;
	}
	
	// Apply the scroll offset by updating the content's position
	m_content->set_position(nanogui::Vector2i(0, -static_cast<int>(m_scroll_offset)));
	
	return true; // Indicate that the event has been handled
}

void FileView::update_visible_items(int first_visible_row) {
	const int columns = 8;
	int start_index = first_visible_row * columns;
	int end_index = start_index + m_visible_rows * columns;
	
	// Clear current items
	clear_file_buttons();
	m_content->shed_children();
	m_item_containers.clear();
	m_image_views.clear();
	m_name_labels.clear();
	m_nodes.clear();
	
	// Reuse item containers if possible
	int total_items = m_all_nodes.size();
	for (int i = start_index; i < end_index && i < total_items; ++i) {
		const auto& child = m_all_nodes[i];
		
		auto item_container = std::make_shared<nanogui::Widget>(*m_content, m_screen);
		item_container->set_fixed_size(nanogui::Vector2i(144, m_row_height));
		m_item_containers.push_back(item_container);
		
		// Acquire a texture from cache
		auto texture = acquire_texture();
		if (!texture) {
			texture = std::make_shared<nanogui::Texture>(nanogui::Texture::PixelFormat::RGBA,
														 nanogui::Texture::ComponentFormat::UInt8,
														 nanogui::Vector2i(512, 512),
														 nanogui::Texture::InterpolationMode::Bilinear,
														 nanogui::Texture::InterpolationMode::Nearest,
														 nanogui::Texture::WrapMode::ClampToEdge);
		}
		
		// Create the image view
		auto image_view = std::make_shared<nanogui::ImageView>(*item_container, m_screen);
		image_view->set_size(nanogui::Vector2i(144, 144));
		image_view->set_fixed_size(nanogui::Vector2i(144, 144));
		image_view->set_image(texture);
		image_view->set_visible(true);
		m_image_views.push_back(image_view);
		
		// Acquire or reuse the button
		auto icon_button = acquire_button(*child, item_container, texture, image_view);
		m_file_buttons.push_back(icon_button);
		m_nodes.push_back(child.get());
		
		// Async thumbnail loading
		m_thumbnail_load_queue.push([this, child, image_view, texture]() {
			load_thumbnail(child, image_view, texture);
		});
		
		// Label
		auto name_label = std::make_shared<nanogui::Label>(*item_container, m_screen, child->FileName);
		name_label->set_fixed_width(144);
		name_label->set_position(nanogui::Vector2i(0, 110));
		name_label->set_alignment(nanogui::Label::Alignment::Center);
		m_name_labels.push_back(name_label);
	}
	
	m_content->perform_layout(m_screen.nvg_context());
}
