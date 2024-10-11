// FileView.cpp
#include "FileView.hpp"

#include <future>
#include <iostream>

FileView::FileView(nanogui::Widget& parent,
				   nanogui::Screen& screen,
				   DirectoryNode& root_directory_node,
				   const std::string& filter_text,
				   const std::set<std::string>& allowed_extensions)
: nanogui::Widget(parent, screen),
m_root_directory_node(root_directory_node),
m_selected_directory_path(m_root_directory_node.FullPath),
m_filter_text(filter_text),
m_selected_button(nullptr),
m_selected_node(nullptr),
m_normal_button_color(nanogui::Color(200, 200, 200, 255)),
m_selected_button_color(nanogui::Color(100, 100, 255, 255)),
m_allowed_extensions(allowed_extensions),
m_scroll_offset(0.0f) {
	
	// Initialize layout
	auto grid_layout = std::unique_ptr<nanogui::AdvancedGridLayout>( new nanogui::AdvancedGridLayout(
																	 /* columns */ std::vector<int>{144, 144, 144, 144, 144, 144, 144, 144}, // 8 columns
																	 /* rows */ {}, // No predefined rows
																	 /* margin */ 8
																	 ));
	
	// Set stretch factors for columns
	for (int i = 0; i < 8; ++i) {
		grid_layout->set_col_stretch(i, 1.0f);
	}
	
	set_layout(std::move(grid_layout));
	
	// Initialize texture cache
	initialize_texture_cache();
	
	// Initial population
	refresh();
}

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
														  nanogui::Vector2i(128, 128),
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
			m_used_textures.erase(m_used_textures.begin());
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

void FileView::refresh(const std::string& filter_text) {
	std::lock_guard<std::mutex> lock(m_mutex);
	m_filter_text = filter_text;
	
	// Clear existing buttons and selection
	clear_file_buttons();
	m_selected_button = nullptr;
	m_selected_node = nullptr;
	
	// Refresh the root directory node to get the latest contents
	m_root_directory_node.refresh(m_allowed_extensions);
	
	// Clear existing child widgets
	shed_children();
	
	auto grid_layout = dynamic_cast<nanogui::AdvancedGridLayout*>(&this->layout());
	if (grid_layout) {
		grid_layout->shed_anchor();
	}
	
	// Clear stored references to previously allocated objects
	m_item_containers.clear();
	m_image_views.clear();
	m_name_labels.clear();
	
	// Populate the file view with updated contents
	populate_file_view();
	
	// Perform layout to apply all changes
	perform_layout(screen().nvg_context());
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
	m_file_buttons.clear();
}

void FileView::populate_file_view() {
	if (m_selected_directory_path.empty()) {
		return;
	}
	
	// Find the currently selected directory node
	DirectoryNode* selected_node = find_node_by_path(m_root_directory_node, m_selected_directory_path);
	
	if (!selected_node) {
		return;
	}
	
	selected_node->refresh(m_allowed_extensions);
	
	const int columns = 8; // 8 columns
	int current_index = 0;
	
	for (const auto& child : selected_node->Children) {
		// Apply filter if any
		if (!m_filter_text.empty()) {
			std::string filename = child->FileName;
			std::string filter = m_filter_text;
			
			// Convert both filename and filter to lowercase for case-insensitive comparison
			std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
			std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);
			
			// Skip files that do not match the filter
			if (filename.find(filter) == std::string::npos) {
				continue;
			}
		}
		
		// Create a container widget for the file item
		auto item_container = std::make_shared<nanogui::Widget>(std::nullopt, screen());
		
		item_container->set_theme(theme());
		item_container->set_fixed_size(nanogui::Vector2i(150, 150));
		m_item_containers.push_back(item_container);
		
		// Calculate grid position based on current index and number of columns
		int col = current_index % columns;
		int row = current_index / columns;
		
		// Define the anchor for this item in the grid
		nanogui::AdvancedGridLayout::Anchor anchor(
												   col, row, 1, 1,
												   nanogui::Alignment::Middle,
												   nanogui::Alignment::Middle
												   );
		
		// Retrieve the grid layout from this widget
		auto grid_layout = dynamic_cast<nanogui::AdvancedGridLayout*>(&this->layout());
		if (grid_layout) {
			grid_layout->set_anchor(*item_container, std::move(anchor));
		}
		
		// Determine the appropriate icon for the file type
		int file_icon = get_icon_for_file(*child);
		
		// Acquire a texture from the cache
		auto texture = acquire_texture();
		if (!texture) {
			// Handle texture exhaustion by creating a new placeholder texture
			texture = std::make_shared<nanogui::Texture>(
															  nanogui::Texture::PixelFormat::RGBA,       // Set pixel format to RGBA
															  nanogui::Texture::ComponentFormat::UInt8,  // Use unsigned 8-bit components for each channel
															  nanogui::Vector2i(128, 128),
															  nanogui::Texture::InterpolationMode::Bilinear,
															  nanogui::Texture::InterpolationMode::Nearest,
															  nanogui::Texture::WrapMode::ClampToEdge
															  );
		}
		
		// Create the icon button within the item container
		auto icon_button = std::make_shared<nanogui::Button>(*item_container, screen(), "", file_icon);
		icon_button->set_fixed_size(nanogui::Vector2i(128, 128));
		icon_button->set_background_color(m_normal_button_color);
		m_file_buttons.push_back(icon_button);
		
		// Create the image view and assign the texture
		auto image_view = std::make_shared<nanogui::ImageView>(*icon_button, screen());
		image_view->set_size(icon_button->fixed_size());
		image_view->set_fixed_size(icon_button->fixed_size());
		image_view->set_image(texture);
		image_view->set_visible(true);
		m_image_views.push_back(image_view);
		
		// Asynchronously load the thumbnail if necessary
		load_thumbnail(child, image_view, texture);
		
		// Add a label below the icon to display the file name
		auto name_label = std::make_shared<nanogui::Label>(*item_container, screen(), child->FileName);
		name_label->set_fixed_width(128);
		name_label->set_position(nanogui::Vector2i(0, 110));
		name_label->set_alignment(nanogui::Label::Alignment::Center);
		m_name_labels.push_back(name_label);
		
		// Set the callback for the icon button to handle interactions
		icon_button->set_callback([this, icon_button, child]() {
			std::lock_guard<std::mutex> lock(m_mutex);
			int file_icon = get_icon_for_file(*child);
			
			if (file_icon == FA_WALKING || file_icon == FA_PERSON_BOOTH || file_icon == FA_OBJECT_GROUP) {
				auto drag_widget = screen().drag_widget();
				
				auto content = std::make_shared<nanogui::ImageView>(*drag_widget, screen());
				content->set_size(icon_button->fixed_size());
				content->set_fixed_size(icon_button->fixed_size());
				
				if (file_icon == FA_PERSON_BOOTH) {
					// Using a simple image icon for now
					content->set_image(std::make_shared<nanogui::Texture>(
																		  "internal/ui/animation.png",
																		  nanogui::Texture::InterpolationMode::Nearest,
																		  nanogui::Texture::InterpolationMode::Nearest,
																		  nanogui::Texture::WrapMode::ClampToEdge
																		  ));
				} else {
					// Assuming texture contains the thumbnail
					content->set_image(dynamic_cast<nanogui::ImageView&>(icon_button->children()[0].get()).image());
				}
				
				content->image()->resize(nanogui::Vector2i(288, 288));
				content->set_visible(true);
				drag_widget->set_size(icon_button->fixed_size());
				
				auto drag_start_position = icon_button->absolute_position();
				drag_widget->set_position(drag_start_position);
				
				screen().set_drag_widget(drag_widget, [this, content, drag_widget, child]() {
					std::lock_guard<std::mutex> lock(m_mutex);
					auto path = child->FullPath;
					
					// Remove drag widget
					drag_widget->shed_children();
					screen().set_drag_widget(nullptr, nullptr);
					
					std::vector<std::string> path_vector = { path };
					screen().drop_event(*this, path_vector);
				});
			}
			
			// Handle selection
			if (m_selected_button && m_selected_button != icon_button.get()) {
				m_selected_button->set_background_color(m_normal_button_color);
			}
			m_selected_button = icon_button.get();
			icon_button->set_background_color(m_selected_button_color);
			
			// Store the selected node for later use
			m_selected_node = child.get();
			
			// Handle double-click for action
			static std::chrono::time_point<std::chrono::high_resolution_clock> last_click_time = std::chrono::high_resolution_clock::time_point::min();
			auto current_click_time = std::chrono::high_resolution_clock::now();
			
			if (last_click_time != std::chrono::high_resolution_clock::time_point::min()) {
				auto click_duration = std::chrono::duration_cast<std::chrono::milliseconds>(current_click_time - last_click_time).count();
				
				if (click_duration < 400) { // 400 ms threshold for double-click
					// Double-click detected
					nanogui::async([this]() {
						std::lock_guard<std::mutex> lock(m_mutex);
						if (m_selected_node) {
							handle_file_interaction(*m_selected_node);
						}
					});
					
					// Reset the double-click state
					last_click_time = std::chrono::high_resolution_clock::time_point::min();
				} else {
					// Update last_click_time for the next click
					last_click_time = current_click_time;
				}
			} else {
				// First click: update last_click_time
				last_click_time = current_click_time;
			}
		});
		
		// Add the item container as a child widget
		this->add_child(*item_container.get());
		
		// Increment the index for the next item
		current_index++;
	}
	
	// After adding all items, adjust the grid layout rows based on the number of items
	if (auto grid_layout = dynamic_cast<nanogui::AdvancedGridLayout*>(&this->layout())) {
		int total_items = m_file_buttons.size();
		int required_rows = (total_items + columns - 1) / columns; // Ceiling division
		
		// Append rows if the current number of rows is less than required
		while (grid_layout->row_count() < required_rows) {
			grid_layout->append_row(150, 1.0f); // Example row height and stretch factor
		}
		
		// Optionally, remove excess rows if there are fewer items
		while (grid_layout->row_count() > required_rows) {
			grid_layout->shed_row();
		}
	}
	
	perform_layout(screen().nvg_context());
}

int FileView::get_icon_for_file(const DirectoryNode& node) const {
	// No shared resources accessed here; mutex not required
	if (node.IsDirectory) return FA_FOLDER;
	if (node.FileName.find(".psk") != std::string::npos) return FA_WALKING;
	if (node.FileName.find(".pma") != std::string::npos)
		return FA_OBJECT_GROUP;
	if (node.FileName.find(".pan") != std::string::npos) return FA_PERSON_BOOTH;
	
	// More conditions for other file types...
	return FA_FILE; // Default icon
}

DirectoryNode* FileView::find_node_by_path(DirectoryNode& root, const std::string& path) const {
	// Recursive function; protect with mutex if called from multiple threads
	
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

void FileView::load_thumbnail(const std::shared_ptr<DirectoryNode>& node,
							  const std::shared_ptr<nanogui::ImageView>& image_view,
							  const std::shared_ptr<nanogui::Texture>& texture) {
	std::async(std::launch::async, [this, node, image_view, texture]() {
		// Load and process the thumbnail
		// Example: Read image data from file
		std::vector<uint8_t> thumbnail_data = load_image_data(node->FullPath);
		
		// Update the texture on the main thread
		nanogui::async([this, image_view, texture, thumbnail_data]() {
			if (!thumbnail_data.empty()) {
				texture->upload(thumbnail_data.data());
				image_view->set_image(texture);
				image_view->set_visible(true);
				perform_layout(screen().nvg_context());
			}
		});
	});
}

std::vector<uint8_t> FileView::load_image_data(const std::string& path) {
	// Implement image loading logic (e.g., using stb_image or another library)
	// Placeholder implementation: Return a solid color image
	// In a real scenario, you would load the image from the file system
	int width = 128;
	int height = 128;
	std::vector<uint8_t> data(width * height * 4, 255); // White texture with full opacity
	return data;
}

bool FileView::scroll_event(const nanogui::Vector2i& p, const nanogui::Vector2f& rel) {
	// Handle scroll events to implement texture wrapping
	// rel.y() > 0 means scrolling down, rel.y() < 0 means scrolling up
	
	if (rel.y() > 0.0f) {
		// Scrolling down
		wrap_texture_cache(true);
	} else if (rel.y() < 0.0f) {
		// Scrolling up
		wrap_texture_cache(false);
	}
	
	return Widget::scroll_event(p, rel);
}

void FileView::wrap_texture_cache(bool scroll_down) {
	if (scroll_down) {
		// Move the last 8 textures to the front
		for (int i = 0; i < m_textures_per_wrap && !m_texture_cache.empty(); ++i) {
			auto texture = m_texture_cache.back();
			m_texture_cache.pop_back();
			m_texture_cache.push_front(texture);
		}
	} else {
		// Move the first 8 textures to the back
		for (int i = 0; i < m_textures_per_wrap && !m_texture_cache.empty(); ++i) {
			auto texture = m_texture_cache.front();
			m_texture_cache.pop_front();
			m_texture_cache.push_back(texture);
		}
	}
}


