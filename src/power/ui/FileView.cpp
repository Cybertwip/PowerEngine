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
m_root_directory_node(root_directory_node),
m_selected_directory_path(root_directory_node.FullPath),
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

void FileView::set_selected_directory_path(const std::string& path) {
	m_selected_directory_path = path;
	refresh();
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

DirectoryNode* FileView::find_node_by_path(DirectoryNode& root, const std::string& path) const {
	if (root.FullPath == path) return &root;
	for (const auto& child : root.Children) {
		if (child->IsDirectory) {
			if (DirectoryNode* found = find_node_by_path(*child, path)) return found;
		}
	}
	return nullptr;
}

void FileView::handle_file_interaction(DirectoryNode& node) {
	if (node.IsDirectory) {
		m_filter_text = "";
		m_root_directory_node = node;
		set_selected_directory_path(node.FullPath);
	} else {
		if (mOnFileSelected) mOnFileSelected(node.shared_from_this());
	}
}

void FileView::handle_file_click(DirectoryNode& node) {
	if (mOnFileClicked) mOnFileClicked(node.shared_from_this());
}

void FileView::refresh() {
	m_content_panel->shed_children();
	m_selected_button = nullptr;
	m_selected_node = nullptr;
	m_root_directory_node.refresh();
	populate_file_view();
	perform_layout(screen().nvg_context());
}

void FileView::populate_file_view() {
	DirectoryNode* selected_node = find_node_by_path(m_root_directory_node, m_selected_directory_path);
	if (!selected_node) {
		selected_node = &m_root_directory_node;
		m_selected_directory_path = selected_node->FullPath;
	}
	
	selected_node->refresh(m_allowed_extensions);
	
	std::vector<std::shared_ptr<DirectoryNode>> collected_nodes;
	if (m_recursive) {
		collect_nodes_recursive(selected_node, collected_nodes);
	} else {
		collected_nodes = selected_node->Children;
	}
	
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
	for (const auto& child : node->Children) {
		if (!m_allowed_extensions.empty() && !child->IsDirectory) {
			std::string extension = child->FileName.substr(child->FileName.find_last_of('.') + 1);
			if (m_allowed_extensions.find(extension) == m_allowed_extensions.end()) continue;
		}
		collected_nodes.push_back(child);
		if (child->IsDirectory) {
			collect_nodes_recursive(child.get(), collected_nodes);
		}
	}
}

void FileView::ProcessEvents() {
	// No async processing needed
}
