#include "FileView.hpp"

#include <nanogui/layout.h>
#include <nanogui/button.h>
#include <nanogui/label.h>
#include <nanogui/imageview.h>

#include <nanovg.h>

#include <future>
#include <iostream>
#include <algorithm>
#include <memory>
#include <mutex>
#include <chrono>

// Constructor
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
m_scroll_offset(0.0f),
m_total_rows(0),
m_visible_rows(3),
m_row_height(144), // Assuming row height of 144 pixels
m_total_textures(40) // Example value, adjust as needed
{
	// Initialize main layout using GridLayout
	auto grid_layout = std::make_unique<nanogui::GridLayout>(
															 nanogui::Orientation::Horizontal,
															 8, // Number of columns
															 nanogui::Alignment::Middle,
															 8, // Margin
															 8  // Spacing between widgets
															 );
	
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
	
	// Reset scroll offset
	m_scroll_offset = 0.0f;
	
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
	
	const int columns = 8; // Number of columns defined in GridLayout
	const int MAX_ROWS = 4; // Maximum number of rows to display
	const int MAX_ITEMS = columns * MAX_ROWS; // Maximum number of items to display
	int current_index = 0;
	m_total_rows = 0; // Reset total rows
	
	for (const auto& child : selected_node->Children) {
		// Check if maximum number of items has been reached
		if (current_index >= MAX_ITEMS) {
			break; // Exit the loop to limit to MAX_ROWS
		}
		
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
		
		// Create a container widget for the file item within the content
		auto item_container = std::make_shared<nanogui::Widget>(*m_content, screen());
		item_container->set_fixed_size(nanogui::Vector2i(144, m_row_height));
		// item_container->set_background_color(nanogui::Color(255, 255, 255, 255)); // Optional: Set background color
		
		m_item_containers.push_back(item_container);
		
		// Add the item container to the GridLayout
		// m_content->add_child(*item_container);
		
		// Determine the appropriate icon for the file type
		int file_icon = get_icon_for_file(*child);
		
		// Acquire a texture from the cache
		auto texture = acquire_texture();
		if (!texture) {
			// Handle texture exhaustion by creating a new placeholder texture
			texture = std::make_shared<nanogui::Texture>(
														 nanogui::Texture::PixelFormat::RGBA,
														 nanogui::Texture::ComponentFormat::UInt8,
														 nanogui::Vector2i(512, 512),
														 nanogui::Texture::InterpolationMode::Bilinear,
														 nanogui::Texture::InterpolationMode::Nearest,
														 nanogui::Texture::WrapMode::ClampToEdge
														 );
		}
		
		// Create the icon button within the item container
		auto icon_button = std::make_shared<nanogui::Button>(*item_container, screen(), "", file_icon);
		icon_button->set_fixed_size(nanogui::Vector2i(144, 144));
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
		m_thumbnail_load_queue.push([this, &child, image_view, texture]() {
			load_thumbnail(child, image_view, texture);
		});

		// Add a label below the icon to display the file name
		auto name_label = std::make_shared<nanogui::Label>(*item_container, screen(), child->FileName);
		name_label->set_fixed_width(144);
		name_label->set_position(nanogui::Vector2i(0, 110));
		name_label->set_alignment(nanogui::Label::Alignment::Center);
		m_name_labels.push_back(name_label);
		
		// Set the callback for the icon button to handle interactions
		icon_button->set_callback([this, icon_button, image_view, child]() {
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
					auto texture = std::make_shared<nanogui::Texture>(
																	  nanogui::Texture::PixelFormat::RGBA,       // Set pixel format to RGBA
																	  nanogui::Texture::ComponentFormat::UInt8,  // Use unsigned 8-bit components for each channel
																	  nanogui::Vector2i(512, 512),
																	  nanogui::Texture::InterpolationMode::Bilinear,
																	  nanogui::Texture::InterpolationMode::Nearest,
																	  nanogui::Texture::WrapMode::ClampToEdge
																	  );
					
					std::vector<uint8_t> thumbnail_data = load_image_data(child->FullPath);
					nanogui::Texture::decompress_into(thumbnail_data, *texture);
					
					content->set_image(texture);
					content->image()->resize(nanogui::Vector2i(288, 288));
				}
				
				content->set_visible(true);
				drag_widget->set_size(icon_button->fixed_size());
				
				auto drag_start_position = icon_button->absolute_position();
				drag_widget->set_position(drag_start_position);
				
				screen().set_drag_widget(drag_widget, [this, thumbnail = std::move(content), drag_widget, child]() {
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
		
		// Increment the index for the next item
		current_index++;
		m_total_rows = std::max(m_total_rows, (int)((current_index + columns - 1) / columns));
	}
	
	// Ensure that m_total_rows does not exceed MAX_ROWS
	m_total_rows = std::min(m_total_rows, MAX_ROWS);
	
	// Optionally, adjust the content size or layout based on m_total_rows
	// For example:
	// m_content->set_fixed_size(nanogui::Vector2i(m_content->fixed_size().x(), m_total_rows * m_row_height));
}

// Determine the appropriate icon for the file type
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

// Recursive function to find a node by its path
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
			perform_layout(screen().nvg_context());
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
	
	auto intersectionSize = parent()->get().size() - position();
	nvgIntersectScissor(ctx, 0, 0, intersectionSize.x(), intersectionSize.y());
	
	for (auto& view : m_image_views) {
		view->set_scissor_rect(absolute_position(), intersectionSize);
	}

	// Draw scissor boundary for debugging
	nvgBeginPath(ctx);
	nvgRect(ctx, 0, 0, width(), height());
	nvgStrokeColor(ctx, nanogui::Color(255, 0, 0, 255));
	nvgStrokeWidth(ctx, 2.0f);
	nvgStroke(ctx);
	
	if (m_content->visible())
		m_content->draw(ctx);
	
	nvgRestore(ctx);
}

bool FileView::scroll_event(const nanogui::Vector2i& p, const nanogui::Vector2f& rel) {
	std::lock_guard<std::mutex> lock(m_mutex);
	
	// Define scroll sensitivity
	float scroll_sensitivity = 10.0f; // Pixels per scroll unit
	
	float scroll_delta = rel.y() * scroll_sensitivity;
	
	// Update scroll offset based on vertical scroll
	m_scroll_offset -= scroll_delta;
	
	// Accumulate scroll delta
	m_accumulated_scroll_delta -= scroll_delta;

	// Clamp the scroll offset to valid range
	int content_height = m_total_rows * m_row_height;
	int view_height = this->height();
	
	if (content_height > view_height) {
		if (m_scroll_offset < 0.0f) {
			m_scroll_offset = 0.0f;
			m_accumulated_scroll_delta = 0.0f;
		}
		if (m_scroll_offset > (content_height - view_height)) {
			m_accumulated_scroll_delta = 0.0f;
			m_scroll_offset = content_height - view_height;
		}
	} else {
		m_scroll_offset = 0.0f;
		m_accumulated_scroll_delta = 0.0f;
	}
	
	
	// Check if accumulated delta exceeds the threshold
	if (std::abs(m_accumulated_scroll_delta) >= m_scroll_threshold) {
		// Reset accumulated delta
		m_accumulated_scroll_delta = 0.0f;
		
		// Perform texture reuse and update the view
		reuse_textures();
	}
	
	
	// Apply the scroll offset by updating the content's position
	m_content->set_position(nanogui::Vector2i(0, -static_cast<int>(m_scroll_offset)));
	
	// Request a redraw if necessary
	// screen().perform_layout();
	// screen().draw_all();
	
	return true; // Indicate that the event has been handled
}

void FileView::reuse_textures() {
	// Determine the first and last visible rows based on the current scroll offset
	int first_visible_row = static_cast<int>(m_scroll_offset / m_row_height);
	int last_visible_row = first_visible_row + m_visible_rows;
	
	// Ensure the indices are within valid range
	first_visible_row = std::max(0, first_visible_row);
	last_visible_row = std::min(m_total_rows, last_visible_row);
	
	// Iterate through the visible rows and update textures as needed
	for (int row = first_visible_row; row < last_visible_row; ++row) {
		for (int col = 0; col < 8; ++col) { // Assuming 8 columns
			int index = row * 8 + col;
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
			
			// Optionally release the old texture if you have a reference to it
			// release_texture(old_texture);
		}
	}
	
	// Optionally perform layout updates
	perform_layout(screen().nvg_context());
}

DirectoryNode* FileView::get_node_by_index(int index) const {
	if (index < 0 || index >= m_file_buttons.size()) {
		return nullptr;
	}
	return m_root_directory_node.Children[index].get();
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
