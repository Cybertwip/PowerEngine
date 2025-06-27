#include "FileView.hpp"

#include <nanogui/layout.h>
#include <nanogui/screen.h>
#include <nanogui/icons.h>
#include <nanogui/button.h>
#include <nanogui/imageview.h>

#include <nanovg.h>

#include <future>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <vector>

#include "filesystem/DirectoryNode.hpp" // Assuming this is needed for load_image_data
#include "filesystem/CompressedSerialization.hpp" // Assuming this is needed for load_image_data
#include "filesystem/ImageUtils.hpp" // Assuming this is needed for load_image_data

// Constructor
FileView::FileView(nanogui::Widget& parent,
				   DirectoryNode& root_directory_node,
				   bool recursive,
				   std::function<void(std::shared_ptr<DirectoryNode>)> onFileClicked,
				   std::function<void(std::shared_ptr<DirectoryNode>)> onFileSelected,
				   int columns,
				   const std::string& filter_text,
				   const std::set<std::string>& allowed_extensions)
: nanogui::Widget(std::make_optional<std::reference_wrapper<Widget>>(parent)),
m_root_directory_node(root_directory_node),
m_selected_directory_path(root_directory_node.FullPath),
m_filter_text(filter_text),
m_allowed_extensions(allowed_extensions),
m_columns(columns),
mOnFileClicked(onFileClicked),
mOnFileSelected(onFileSelected),
m_recursive(recursive),
m_selected_button(nullptr),
m_selected_node(nullptr),
m_normal_button_color(200, 200, 200, 255),
m_selected_button_color(100, 100, 255, 255),
m_scroll_offset(0.0f),
m_accumulated_scroll_delta(0.0f),
m_total_rows(0),
m_visible_rows(3),
m_row_height(144)
{
	set_layout(std::make_unique<nanogui::GridLayout>(nanogui::Orientation::Horizontal, m_columns, nanogui::Alignment::Middle, 8, 8));
	
	// Content widget to hold the scrollable items
	m_content = std::make_shared<nanogui::Widget>(std::make_optional<std::reference_wrapper<nanogui::Widget>>(*this));
	m_content->set_layout(std::make_unique<nanogui::GridLayout>(nanogui::Orientation::Horizontal, m_columns, nanogui::Alignment::Middle, 8, 8));
	
	// Initial population of the view
	refresh();
	
	m_drag_payload = std::make_shared<nanogui::ImageView>(*this, screen());
	m_drag_payload->set_visible(false);
	// Keep the widget alive, but remove it from the layout tree for now.
	// It will be temporarily attached to the screen's drag widget during a drag operation.
	this->remove_child(*m_drag_payload);
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
			m_used_textures.push_back(texture); // Keep it tracked
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
std::shared_ptr<nanogui::Button> FileView::acquire_button(const std::shared_ptr<DirectoryNode>& child) {
	// 1. Create item container using the specified constructor pattern.
	// This adds the widget to m_content and returns a shared_ptr.
	auto item_container = std::make_shared<nanogui::Widget>(std::make_optional<std::reference_wrapper<nanogui::Widget>>(*m_content));
	item_container->set_fixed_size({144, m_row_height});
	item_container->set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Middle, 0, 5));
	m_item_containers.push_back(item_container); // Store pointer to manage lifetime
	
	// 2. Create the main button as a child of the container.
	auto icon_button = std::shared_ptr<nanogui::Button>(new nanogui::Button(*item_container, "", get_icon_for_file(*child)));
	icon_button->set_fixed_size({144, 110});
	icon_button->set_background_color(m_normal_button_color);
	m_file_buttons.push_back(icon_button); // Store pointer
	
	// 3. Create the image view. Note the parenting change below.
	auto image_view = std::shared_ptr<nanogui::ImageView>(new nanogui::ImageView(*icon_button, screen())); // Parented to button
	image_view->set_size({128, 128});
	m_image_views.push_back(image_view); // Store pointer
	
	// 4. Create the label as a child of the icon_button.
	auto name_label = std::make_shared<nanogui::Label>(*icon_button, child->FileName);
	name_label->set_fixed_width(144);
	// Position the label at the bottom of the button area.
	name_label->set_position({0, static_cast<int>(icon_button->height() * 0.8)});
	name_label->set_alignment(nanogui::Label::Alignment::Center);
	m_name_labels.push_back(name_label); // Store pointer
	
	// Asynchronously load the thumbnail into the image view
	load_thumbnail(child, image_view);
	
	// Setup all interaction logic (click, double-click, drag)
	setup_button_interactions(icon_button, child);
	
	return icon_button;
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

int FileView::get_icon_for_file(const DirectoryNode& node) const {
	if (node.IsDirectory) return FA_FOLDER;
	if (node.FileName.find(".psk") != std::string::npos) return FA_WALKING;
	if (node.FileName.find(".pma") != std::string::npos) return FA_OBJECT_GROUP;
	if (node.FileName.find(".pan") != std::string::npos) return FA_PERSON_BOOTH;
	if (node.FileName.find(".png") != std::string::npos) return FA_PHOTO_VIDEO;
	
	// More conditions for other file types...
	return FA_FILE; // Default icon
}

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

void FileView::handle_file_interaction(DirectoryNode& node) {
	if (node.IsDirectory) {
		// Change the selected directory path and refresh
		
		m_filter_text = "";
		
		m_root_directory_node = node;
		
		set_selected_directory_path(node.FullPath);
		
	} else {
		if (mOnFileSelected) {
			mOnFileSelected(node.shared_from_this());
		}
	}
}


void FileView::handle_file_click(DirectoryNode& node) {
	if (!node.IsDirectory) {
		if (mOnFileClicked) {
			mOnFileClicked(node.shared_from_this());
		}
	}
}
void FileView::load_thumbnail(const std::shared_ptr<DirectoryNode>& node, std::shared_ptr<nanogui::ImageView> image_view) {
	nanogui::async([this, node, image_view]() {
		std::vector<uint8_t> thumbnail_data;
		if (get_icon_for_file(*node) == FA_PHOTO_VIDEO) {
			thumbnail_data = load_file_to_vector(node->FullPath);
		} else {
			thumbnail_data = load_image_data(node->FullPath);
		}
		
		if (!thumbnail_data.empty()) {
			auto texture = std::make_shared<nanogui::Texture>(
															  nanogui::Texture::PixelFormat::RGBA,
															  nanogui::Texture::ComponentFormat::UInt8,
															  nanogui::Vector2i(512, 512)
															  );
			nanogui::Texture::decompress_into(thumbnail_data, *texture);
			
			// Update the UI on the main thread
			perform_layout(screen().nvg_context());
			image_view->set_image(texture);
		}
	});
}

std::vector<uint8_t> FileView::load_image_data(const std::string& path) {
	CompressedSerialization::Deserializer deserializer;
	deserializer.initialize_header_from_file(path);
	
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
void FileView::refresh() {
	// Clear existing items before repopulating
	m_content->shed_children();
	m_selected_button = nullptr;
	m_selected_node = nullptr;

	m_root_directory_node.refresh();
	
	populate_file_view();
	
	// Redraw the screen with the new content
	perform_layout(screen().nvg_context());
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
	
	const int columns = m_columns;
	int current_index = 0;
	
	m_nodes.clear();
	
	// Collect nodes
	std::vector<std::shared_ptr<DirectoryNode>> collected_nodes;
	if (m_recursive) {
		collect_nodes_recursive(selected_node, collected_nodes);
	} else {
		collected_nodes = selected_node->Children;
	}
	
	
	// Filtered list of children
	std::vector<std::shared_ptr<DirectoryNode>> filtered_children;
	
	for (const auto& child : collected_nodes) {
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
	
	m_all_nodes = filtered_children; // Store all filtered nodes for future use
	m_total_rows = (filtered_children.size() + columns - 1) / columns;
	
	// Only load the initial visible rows
	int max_items = m_visible_rows * columns;
	
	for (int i = 0; i < std::min(static_cast<int>(filtered_children.size()), max_items); ++i) {
		const auto& child = filtered_children[i];
		
		// Acquire or reuse the button and associated UI elements
		acquire_button(child);
		
		// Store the node pointer
		m_nodes.push_back(child.get());
	}
	
	
	if (filtered_children.empty()) {
		m_dummy_widget = std::make_shared<nanogui::Widget>(std::make_optional<std::reference_wrapper<nanogui::Widget>>(*m_content));
		
		m_dummy_widget->set_size(nanogui::Vector2i(1, 1));
	}
}


bool FileView::scroll_event(const nanogui::Vector2i& p, const nanogui::Vector2f& rel) {
	std::lock_guard<std::mutex> lock(m_mutex);
	
	float scroll_delta = rel.y();
	
	scroll_delta = glm::sign(scroll_delta) * std::max(std::abs(scroll_delta), 4.0f); // clamp max 1/2 row per scroll
	
	// Update scroll offset
	m_scroll_offset -= scroll_delta;
	
	// Clamp the scroll offset
	int content_height = m_total_rows * m_row_height;
	int view_height = this->height();
	
	if (content_height > view_height) {
		if (m_scroll_offset < 0.0f) {
			m_scroll_offset = 0.0f;
		}
		if (m_scroll_offset > content_height - view_height) {
			m_scroll_offset = content_height - view_height;
		}
	} else {
		m_scroll_offset = 0.0f;
	}
	
	// Determine scroll direction
	int direction = 0; // 1 for down, -1 for up
	if (m_scroll_offset > m_previous_scroll_offset) {
		direction = 1; // Scrolling down
	} else if (m_scroll_offset < m_previous_scroll_offset) {
		direction = -1; // Scrolling up
	}
	m_previous_scroll_offset = m_scroll_offset;
	
	// Calculate the first visible row
	int new_first_visible_row = static_cast<int>(m_scroll_offset / m_row_height);
	
	// Check if we need to update visible items
	if (new_first_visible_row != m_previous_first_visible_row) {
		// Update visible items based on scroll direction
		
		update_visible_items(new_first_visible_row, direction);
		m_previous_first_visible_row = new_first_visible_row;
	}
	
	// Apply the scroll offset by updating the content's position
	m_content->set_position(nanogui::Vector2i(0, -static_cast<int>(m_scroll_offset)));	return true; // Indicate that the event has been handled
	
	m_content->perform_layout(screen().nvg_context());
}

void FileView::update_visible_items(int first_visible_row, int direction) {
	const int columns = m_columns;
	int start_index;
	int end_index;
	
	if (direction > 0) { // Scrolling down
		// Load the row after the last visible row
		start_index = (first_visible_row + m_visible_rows) * columns;
		end_index = start_index + columns; // Load one additional row
	} else if (direction < 0) { // Scrolling up
		// Load the row before the first visible row
		start_index = (first_visible_row - 1) * columns;
		end_index = start_index + columns; // Load one additional row
	} else {
		// No direction change; do not update
		return;
	}
	
	// Ensure start_index is within bounds
	if (start_index < 0) {
		start_index = 0;
	}
	if (start_index >= static_cast<int>(m_all_nodes.size())) {
		return; // Nothing to load
	}
	
	// Ensure end_index is within bounds
	if (end_index > static_cast<int>(m_all_nodes.size())) {
		end_index = static_cast<int>(m_all_nodes.size());
	}
	
	// Loop through the range and update/reuse widgets
	for (int i = start_index; i < end_index; ++i) {
		// Check if the item is already visible
		if (i >= first_visible_row * columns && i < (first_visible_row + m_visible_rows) * columns) {
			continue; // Already visible
		}
		
		// Otherwise, update or add the new item
		const auto& child = m_all_nodes[i];
		
		// Reuse existing widgets if possible
		add_or_update_widget(i, child);
	}
	
	// Optionally remove widgets that are no longer visible
	remove_invisible_widgets(first_visible_row);
	
	m_content->perform_layout(screen().nvg_context());
}

void FileView::add_or_update_widget(int index, const std::shared_ptr<DirectoryNode>& child) {
	acquire_button(child);
}

void FileView::remove_invisible_widgets(int first_visible_row) {
	const int columns = m_columns;
	int start_visible = first_visible_row * columns;
	int end_visible = (first_visible_row + m_visible_rows) * columns;
	
	// Iterate through file buttons and release those outside the visible range
	for (int i = m_file_buttons.size() - 1; i >= 0; --i) {
		// Only remove buttons that are beyond the end_visible index
		if (i >= end_visible) {
			m_file_buttons.erase(m_file_buttons.begin() + i);
		}
	}
	
	// Similarly handle image views
	for (int i = m_image_views.size() - 1; i >= 0; --i) {
		if (i >= end_visible) {
			release_texture(m_image_views[i]->image());
			m_image_views[i]->set_visible(false);
			m_image_views.erase(m_image_views.begin() + i);
		}
	}
	
	// Similarly handle labels
	for (int i = m_name_labels.size() - 1; i >= 0; --i) {
		if (i >= end_visible) {
			m_name_labels[i]->set_visible(false);
			m_name_labels.erase(m_name_labels.begin() + i);
		}
	}
	
	// Similarly handle item containers
	for (int i = m_item_containers.size() - 1; i >= 0; --i) {
		if (i >= end_visible) {
			m_item_containers[i]->set_visible(false);
			m_content->remove_child(*m_item_containers[i]);
			m_item_containers.erase(m_item_containers.begin() + i);
		}
	}
}


DirectoryNode* FileView::get_node_by_index(int index) const {
	if (index < 0 || index >= static_cast<int>(m_nodes.size())) {
		return nullptr;
	}
	return m_nodes[index];
}

void FileView::collect_nodes_recursive(DirectoryNode* node, std::vector<std::shared_ptr<DirectoryNode>>& collected_nodes) {
	
	collected_nodes.clear();
	
	for (const auto& child : node->Children) {
		
		// Apply allowed extensions if any
		if (!m_allowed_extensions.empty() && !child->IsDirectory) {
			std::string extension = child->FileName.substr(child->FileName.find_last_of('.') + 1);
			if (m_allowed_extensions.find(extension) == m_allowed_extensions.end()) {
				
				collected_nodes.push_back(child);
			}
		}
		
		if (child->IsDirectory) {
			// Recurse into child directory
			collect_nodes_recursive(child.get(), collected_nodes);
		}
	}
}
void FileView::setup_button_interactions(std::shared_ptr<nanogui::Button> button, const std::shared_ptr<DirectoryNode>& node) {
	// Setup single-click for selection
	button->set_callback([this, button, node]() {
		if (m_selected_button) {
			m_selected_button->set_background_color(m_normal_button_color);
		}
		m_selected_button = button;
		button->set_background_color(m_selected_button_color);
		m_selected_node = node;
		
		if (mOnFileClicked) {
			mOnFileClicked(node);
		}
	});
	
	// Setup double-click (optional, but good practice)
	button->set_double_click_callback([this, node]() {
		handle_file_interaction(*node);
	});
	
	// THIS IS THE MAIN FIX: Use the button's drag callback to initiate the screen drag operation.
	button->set_drag_callback([this, button, node](const nanogui::Vector2i&, const nanogui::Vector2i& rel, int, int) {
		// Check if a drag is not already active and if the mouse has moved enough
		if (!m_is_dragging && (std::abs(rel.x()) > 5 || std::abs(rel.y()) > 5)) {
			// Set a flag to ensure we only initiate the drag once per gesture
			m_is_dragging = true;
			initiate_drag_operation(button, node);
		}
	});
}

// This function now correctly initiates the drag operation at the start of the gesture.
void FileView::initiate_drag_operation(std::shared_ptr<nanogui::Button> button, const std::shared_ptr<DirectoryNode>& node) {
	// 1. Get the screen's dedicated widget for drag operations.
	auto* drag_container = screen().drag_widget();
	if (!drag_container) {
		return;
	}
	
	// 2. Configure the reusable m_drag_payload with the item's image.
	// (This part of your logic was correct)
	auto drag_texture = std::make_shared<nanogui::Texture>(
														   nanogui::Texture::PixelFormat::RGBA,
														   nanogui::Texture::ComponentFormat::UInt8,
														   nanogui::Vector2i(512, 512)
														   );
	
	std::vector<uint8_t> thumbnail_data;
	if (get_icon_for_file(*node) == FA_PHOTO_VIDEO) {
		thumbnail_data = load_file_to_vector(node->FullPath);
	} else {
		thumbnail_data = load_image_data(node->FullPath);
	}
	
	if (!thumbnail_data.empty()) {
		nanogui::Texture::decompress_into(thumbnail_data, *drag_texture);
		m_drag_payload->set_image(drag_texture);
	}
	
	// 3. Set up sizes and add the payload to the screen's drag container.
	m_drag_payload->set_size({128, 128});
	m_drag_payload->set_visible(true);
	
	drag_container->add_child(*m_drag_payload);
	drag_container->set_size(m_drag_payload->size());
	drag_container->set_position(screen().mouse_pos() - m_drag_payload->size() / 2); // Center on cursor
	drag_container->set_visible(true);
	
	// 4. This is the core step: "arm" the screen's drag system with the drop callback.
	screen().set_drag_widget(drag_container, [this, node]() {
		// This lambda is the DROP HANDLER. It's executed when the user releases the mouse.
		std::vector<std::string> paths = { node->FullPath };
		
		// Dispatch the drop event to the main Application class.
		screen().drop_event(*this, paths);
		
		// Clean up: hide the payload and release the screen's drag widget.
		m_drag_payload->set_visible(false);
		drag_container->remove_child(*m_drag_payload); // Important: remove child
		screen().set_drag_widget(nullptr, nullptr);
		
		// Reset our local drag state flag
		m_is_dragging = false;
	});
}
