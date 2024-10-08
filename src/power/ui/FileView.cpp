#include "FileView.hpp"
#include "filesystem/DirectoryNode.hpp"
#include "filesystem/CompressedSerialization.hpp"

#include <algorithm>
#include <chrono>
#include <memory>


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
m_allowed_extensions(allowed_extensions) {
	
	// Initialize layout
	auto grid_layout = std::unique_ptr<nanogui::AdvancedGridLayout>(new nanogui::AdvancedGridLayout(
													  /* columns */ {144, 144, 144, 144, 144, 144, 144, 144}, // Initial column widths (can be adjusted)
													  /* rows */ {},                // Start with no predefined rows
													  /* margin */ 8
													  ));
	
	// Optionally, set stretch factors for columns and rows
	grid_layout->set_col_stretch(0, 1.0f);
	grid_layout->set_col_stretch(1, 1.0f);
	grid_layout->set_col_stretch(2, 1.0f);
	grid_layout->set_col_stretch(3, 1.0f);
	grid_layout->set_col_stretch(4, 1.0f);
	grid_layout->set_col_stretch(5, 1.0f);
	grid_layout->set_col_stretch(6, 1.0f);
	grid_layout->set_col_stretch(7, 1.0f);

	set_layout(std::move(grid_layout));
	
	// Initial population
	refresh();
}

void FileView::refresh(const std::string& filter_text) {
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
	m_selected_directory_path = path;
	refresh();
}

void FileView::set_filter_text(const std::string& filter) {
	m_filter_text = filter;
	refresh();
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
	
	selected_node->refresh(m_allowed_extensions);
	
	if (!selected_node) {
		return;
	}
	
	const int columns = 8; // Adjust as needed for your UI
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
		m_item_containers.push_back(item_container); // Store the reference
		
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
		
		// Create the icon button within the item container
		auto icon_button = std::make_shared<nanogui::Button>(*item_container, screen(), "", file_icon);
		icon_button->set_fixed_size(nanogui::Vector2i(128, 128));
		icon_button->set_background_color(m_normal_button_color);
		m_file_buttons.push_back(icon_button); // Store the reference
		
		// Handle thumbnail deserialization for specific file types
		auto thumbnail_pixels = std::make_shared<std::vector<uint8_t>>();
		
		if (file_icon == FA_WALKING || file_icon == FA_OBJECT_GROUP) {
			// Deserialize thumbnail here
			CompressedSerialization::Deserializer deserializer;
			deserializer.load_from_file(child->FullPath);
			
			thumbnail_pixels->resize(512 * 512 * 4);
			uint64_t thumbnail_size = 0;
			uint64_t hash_id[] = {0, 0};
			
			deserializer.read_header_raw(hash_id, sizeof(hash_id)); // To increase the offset and read the thumbnail size
			deserializer.read_header_uint64(thumbnail_size);
			
			if (thumbnail_size != 0) {
				deserializer.read_header_raw(thumbnail_pixels->data(), thumbnail_size);
				
				auto image_view = std::make_shared<nanogui::ImageView>(*icon_button, screen());
				image_view->set_size(icon_button->fixed_size());
				image_view->set_fixed_size(icon_button->fixed_size());
				
				image_view->set_image(std::make_shared<nanogui::Texture>(
																		 thumbnail_pixels->data(),
																		 thumbnail_pixels->size(),
																		 512, 512,
																		 nanogui::Texture::InterpolationMode::Nearest,
																		 nanogui::Texture::InterpolationMode::Nearest,
																		 nanogui::Texture::WrapMode::ClampToEdge
																		 ));
				image_view->image()->resize(nanogui::Vector2i(288, 288));
				image_view->set_visible(true);
				
				m_image_views.push_back(image_view); // Store the reference
			}
		}
		
		// Add a label below the icon to display the file name
		auto name_label = std::make_shared<nanogui::Label>(*item_container, screen(), child->FileName);
		name_label->set_fixed_width(128);
		name_label->set_position(nanogui::Vector2i(0, 110));
		name_label->set_alignment(nanogui::Label::Alignment::Center);
		m_name_labels.push_back(name_label); // Store the reference
		
		// Set the callback for the icon button to handle interactions
		icon_button->set_callback([this, icon_button, thumbnail_pixels, child]() {
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
					content->set_image(std::make_shared<nanogui::Texture>(
																		  thumbnail_pixels->data(),
																		  thumbnail_pixels->size(),
																		  512, 512,
																		  nanogui::Texture::InterpolationMode::Nearest,
																		  nanogui::Texture::InterpolationMode::Nearest,
																		  nanogui::Texture::WrapMode::ClampToEdge
																		  ));
				}
				
				content->image()->resize(nanogui::Vector2i(288, 288));
				content->set_visible(true);
				drag_widget->set_size(icon_button->fixed_size());
				
				auto drag_start_position = icon_button->absolute_position();
				drag_widget->set_position(drag_start_position);
				drag_widget->perform_layout(screen().nvg_context());
				
				screen().set_drag_widget(drag_widget, [this, content, drag_widget, child]() {
					auto path = child->FullPath;
					
					// Remove drag widget
					drag_widget->remove_child(*content);
					screen().set_drag_widget(nullptr, nullptr);
					
					std::vector<std::string> path_vector = { path };
					screen().drop_event(*this, path_vector);
				});
			}
			
			// Handle selection
			if (m_selected_button && m_selected_button != icon_button) {
				m_selected_button->set_background_color(m_normal_button_color);
			}
			m_selected_button = icon_button;
			icon_button->set_background_color(m_selected_button_color);
			
			// Store the selected node for later use
			m_selected_node = child;
			
			// Handle double-click for action
			static std::chrono::time_point<std::chrono::high_resolution_clock> last_click_time = std::chrono::time_point<std::chrono::high_resolution_clock>::min();
			auto current_click_time = std::chrono::high_resolution_clock::now();
			
			if (last_click_time != std::chrono::time_point<std::chrono::high_resolution_clock>::min()) {
				auto click_duration = std::chrono::duration_cast<std::chrono::milliseconds>(current_click_time - last_click_time).count();
				
				if (click_duration < 400) { // 400 ms threshold for double-click
					// Double-click detected
					nanogui::async([this]() {
						handle_file_interaction(*m_selected_node);
					});
					
					// Reset the double-click state
					last_click_time = std::chrono::time_point<std::chrono::high_resolution_clock>::min();
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
}

int FileView::get_icon_for_file(const DirectoryNode& node) const {
	if (node.IsDirectory) return FA_FOLDER;
	if (node.FileName.find(".psk") != std::string::npos) return FA_WALKING;
	if (node.FileName.find(".pma") != std::string::npos)
		return FA_OBJECT_GROUP;
	if (node.FileName.find(".pan") != std::string::npos) return FA_PERSON_BOOTH;
	
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
		node.refresh(m_allowed_extensions);
		m_selected_directory_path = node.FullPath;
	} else {
		if (node.FileName.find(".seq") != std::string::npos || node.FileName.find(".cmp") != std::string::npos) {
			// Logic for opening sequence or composition
		} else if (node.FileName.find(".fbx") != std::string::npos) {
			//mActorVisualManager.add_actor(mMeshActorLoader.create_actor(node.FullPath, mDummyAnimationTimeProvider, *mMeshShader, *mSkinnedShader));
		}
		// Handle other file type interactions...
	}
	
//	
//	// Reset the selection
//	if (mSelectedButton) {
//		mSelectedButton->set_background_color(mNormalButtonColor);
//		mSelectedButton = nullptr;
//		mSelectedNode = nullptr;
//	}
	
	nanogui::async([this](){
		refresh();
	});
}
