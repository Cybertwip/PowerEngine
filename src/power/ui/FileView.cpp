#include "FileView.hpp"
#include "filesystem/DirectoryNode.hpp"

#include <nanogui/button.h>
#include <nanogui/icons.h>
#include <nanogui/label.h>
#include <nanogui/layout.h>
#include <nanogui/screen.h>
#include <nanogui/vscrollpanel.h>

#include <algorithm>
#include <iostream>

// Constructor
FileView::FileView(
				   nanogui::Widget& parent,
				   DirectoryNode& root_directory_node,
				   bool recursive,
				   std::function<void(std::shared_ptr<DirectoryNode>)> onFileClicked,
				   std::function<void(std::shared_ptr<DirectoryNode>)> onFileSelected,
				   const std::string& filter_text,
				   const std::set<std::string>& allowed_extensions)
: nanogui::Widget(std::make_optional<std::reference_wrapper<Widget>>(parent)),
m_initial_root_node(root_directory_node),
m_current_directory_node(&root_directory_node),
m_filter_text(filter_text),
m_allowed_extensions(allowed_extensions),
m_recursive(recursive),
mOnFileClicked(onFileClicked),
mOnFileSelected(onFileSelected),
m_selected_button(nullptr),
m_selected_node(nullptr),
m_normal_button_color(0, 0), // Transparent background
m_selected_button_color(0, 100, 255, 50) // Semi-transparent blue for selection
{
	set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Fill));
	m_vscroll = std::make_shared<nanogui::VScrollPanel>(*this);
	
	m_content_panel = std::make_shared<nanogui::Widget>(*m_vscroll);
	m_content_panel->set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Fill, 0, 2));
	
	// Create the drag payload widget but keep it detached and invisible
	m_drag_payload = std::make_shared<nanogui::Label>(*this, "");
	m_drag_payload->set_visible(false);
	this->remove_child(*m_drag_payload);
	
	refresh();
}

// Destructor
FileView::~FileView() {
	// Shared pointers and NanoGUI handle cleanup
}

void FileView::set_filter_text(const std::string& filter) {
	m_filter_text = filter;
	refresh();
}

int FileView::get_icon_for_file(const DirectoryNode& node) const {
	if (node.IsDirectory) return FA_FOLDER;
	if (node.FileName.find(".pan") != std::string::npos) return FA_PERSON_BOOTH;
	if (node.FileName.find(".png") != std::string::npos) return FA_IMAGE;
	return FA_FILE;
}

void FileView::handle_file_interaction(DirectoryNode& node) {
	if (node.IsDirectory) {
		m_filter_text = "";
		m_current_directory_node = &node;
		refresh();
	} else {
		if (mOnFileSelected) mOnFileSelected(node.shared_from_this());
	}
}

void FileView::handle_file_click(DirectoryNode& node) {
	if (mOnFileClicked) mOnFileClicked(node.shared_from_this());
}

void FileView::refresh() {
	m_content_panel->shed_children();
	m_file_buttons.clear();
	m_selected_button = nullptr;
	m_selected_node = nullptr;
	m_initial_root_node.refresh();
	populate_file_view();
	perform_layout(screen().nvg_context());
}

void FileView::populate_file_view() {
	if (!m_current_directory_node) {
		m_current_directory_node = &m_initial_root_node;
	}
	
	m_current_directory_node->refresh(m_allowed_extensions);
	
	if (m_current_directory_node != &m_initial_root_node && m_current_directory_node->Parent) {
		m_cd_up_button = std::make_shared<nanogui::Button>(*m_content_panel, "..", FA_ARROW_UP);
		m_cd_up_button->set_fixed_height(28);
		m_cd_up_button->set_icon_position(nanogui::Button::IconPosition::Left);
		m_cd_up_button->set_background_color(m_normal_button_color);
		m_cd_up_button->set_callback([this]() {
			// To access a weak_ptr, you must first 'lock' it to get a temporary shared_ptr.
			// If the parent object still exists, the lock succeeds.
			if (auto parent_ptr = m_current_directory_node->Parent.lock()) {
				// Note: You may need to change m_current_directory_node to be a
				// std::shared_ptr<DirectoryNode> instead of a raw pointer.
				// If so, the assignment would be: m_current_directory_node = parent_ptr;
				m_current_directory_node = parent_ptr.get();
				refresh();
			}
		});
	}
	
	std::vector<std::shared_ptr<DirectoryNode>> collected_nodes;
	if (m_recursive) {
		// This uses the corrected recursive function from the previous answer
		collect_nodes_recursive(m_current_directory_node, collected_nodes);
	} else {
		// CORRECTED non-recursive logic
		for (const auto& child : m_current_directory_node->Children) {
			// Directories are always shown in a non-recursive view.
			if (child->IsDirectory) {
				collected_nodes.push_back(child);
				continue;
			}
			
			// For files, we must manually apply the extension filter.
			if (m_allowed_extensions.empty()) {
				// If no filter is set, add all files.
				collected_nodes.push_back(child);
			} else {
				// Otherwise, check if the file's extension is allowed.
				const auto pos = child->FileName.find_last_of('.');
				if (pos != std::string::npos) {
					std::string extension = child->FileName.substr(pos + 1);
					if (m_allowed_extensions.count(extension)) {
						collected_nodes.push_back(child);
					}
				}
			}
		}
	}
	
	// This loop now receives a correctly filtered list in both cases.
	for (const auto& child : collected_nodes) {
		if (!m_filter_text.empty()) {
			std::string filename = child->FileName;
			std::string filter = m_filter_text;
			std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
			std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);
			if (filename.find(filter) == std::string::npos) continue;
		}
		create_file_item(child);
	}
}
void FileView::create_file_item(const std::shared_ptr<DirectoryNode>& node) {
	auto item_button = std::make_shared<nanogui::Button>(*m_content_panel, node->FileName, get_icon_for_file(*node));
	item_button->set_fixed_height(28);
	item_button->set_icon_position(nanogui::Button::IconPosition::Left);
	item_button->set_background_color(m_normal_button_color);
	item_button->set_flags(nanogui::Button::Flags::ToggleButton);
	
	m_file_buttons.push_back(item_button);
	
	item_button->set_change_callback([this, item_button, node](bool state) {
		if (m_selected_button) {
			m_selected_button->set_background_color(m_normal_button_color);
			m_selected_button->set_pushed(false);
		}
		m_selected_button = item_button;
		m_selected_button->set_background_color(m_selected_button_color);
		m_selected_node = node;
		handle_file_click(*node);
	});
	
	item_button->set_double_click_callback([this, node]() {
		handle_file_interaction(*node);
	});
	
	// Add drag-and-drop functionality
	item_button->set_drag_callback([this, node](const nanogui::Vector2i&, const nanogui::Vector2i& rel, int, int) {
		if (!m_is_dragging && (std::abs(rel.x()) > 5 || std::abs(rel.y()) > 5)) {
			m_is_dragging = true;
			initiate_drag_operation(node);
		}
	});
}

void FileView::initiate_drag_operation(const std::shared_ptr<DirectoryNode>& node) {
	auto* drag_container = screen().drag_widget();
	if (!drag_container) return;
	
	// Configure the payload label
	m_drag_payload->set_caption(node->FileName);
	//	m_drag_payload->set_icon(get_icon_for_file(*node));
	m_drag_payload->set_visible(true);
	m_drag_payload->set_size(m_drag_payload->preferred_size(screen().nvg_context()));
	
	// Attach payload to the screen's drag container
	drag_container->add_child(*m_drag_payload);
	drag_container->set_size(m_drag_payload->size());
	drag_container->set_position(screen().mouse_pos() - m_drag_payload->size() / 2);
	drag_container->set_visible(true);
	
	// Set the drop handler
	screen().set_drag_widget(drag_container, [this, drag_container, node]() {
		// This lambda is the DROP HANDLER
		std::vector<std::string> paths = { node->FullPath };
		screen().drop_event(*this, paths);
		
		// Cleanup
		m_drag_payload->set_visible(false);
		this->add_child(*m_drag_payload); // Re-attach to FileView
		this->remove_child(*m_drag_payload); // Detach again to hide from layout
		
		screen().set_drag_widget(nullptr, nullptr);
		m_is_dragging = false;
	});
}


void FileView::collect_nodes_recursive(DirectoryNode* node, std::vector<std::shared_ptr<DirectoryNode>>& collected_nodes) {
	// Iterate through all children of the current node
	for (const auto& child : node->Children) {
		// If the child is a directory, recurse into it to find files.
		// The directory itself is not added to the list.
		if (child->IsDirectory) {
			collect_nodes_recursive(child.get(), collected_nodes);
		}
		// If the child is a file, check if it should be added.
		else {
			// Case 1: No extension filter is active, so add all files.
			if (m_allowed_extensions.empty()) {
				collected_nodes.push_back(child);
			}
			// Case 2: An extension filter is active.
			else {
				// Safely find the last '.' in the filename.
				const auto pos = child->FileName.find_last_of('.');
				
				// Check that a dot was found and it's not the only character.
				if (pos != std::string::npos) {
					// Extract the extension.
					std::string extension = child->FileName.substr(pos + 1);
					// If the extension is in the allowed set, add the file.
					// std::set::count is an efficient way to check for existence.
					if (m_allowed_extensions.count(extension)) {
						collected_nodes.push_back(child);
					}
				}
			}
		}
	}
}
void FileView::ProcessEvents() {
	// No async processing needed
}
