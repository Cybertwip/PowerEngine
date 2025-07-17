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
m_current_directory_node(&root_directory_node), // MODIFIED: Initialize the pointer
m_filter_text(filter_text),
m_allowed_extensions(allowed_extensions),
m_recursive(recursive),
mOnFileClicked(onFileClicked),
mOnFileSelected(onFileSelected),
m_selected_button(nullptr),
m_selected_node(nullptr),
m_normal_button_color(0, 0),
m_selected_button_color(0, 100, 255, 50)
{
	set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Fill));
	m_vscroll = std::make_shared<nanogui::VScrollPanel>(*this);
	
	m_content_panel = std::make_shared<nanogui::Widget>(*m_vscroll);
	m_content_panel->set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Fill, 0, 2));
	
	m_drag_payload = std::make_shared<nanogui::Label>(*this, "");
	m_drag_payload->set_visible(false);
	this->remove_child(*m_drag_payload);
	
	refresh();
}

FileView::~FileView() {}

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
		m_current_directory_node = &node; // MODIFIED: Point to the new directory
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
	m_selected_button = nullptr;
	m_selected_node = nullptr;
	if (m_current_directory_node) {
		m_current_directory_node->refresh(m_allowed_extensions);
		populate_file_view();
	}
	perform_layout(screen().nvg_context());
}

void FileView::populate_file_view() {
	// MODIFIED: Check address of initial root against the current node pointer
	if (m_current_directory_node != &m_initial_root_node && m_current_directory_node->Parent) {
		auto up_button = std::make_shared<nanogui::Button>(*m_content_panel, "..", FA_ARROW_UP);
		up_button->set_fixed_height(28);
		up_button->set_icon_position(nanogui::Button::IconPosition::Left);
		up_button->set_background_color(m_normal_button_color);
		
		up_button->set_callback([this]() {
			if (m_current_directory_node->Parent) {
				m_current_directory_node = m_current_directory_node->Parent; // MODIFIED: Navigate up
				refresh();
			}
		});
	}
	
	std::vector<std::shared_ptr<DirectoryNode>> collected_nodes;
	if (m_recursive) {
		collect_nodes_recursive(m_current_directory_node, collected_nodes);
	} else {
		collected_nodes = m_current_directory_node->Children;
	}
	
	std::sort(collected_nodes.begin(), collected_nodes.end(), [](const auto& a, const auto& b){
		if (a->IsDirectory != b->IsDirectory) return a->IsDirectory;
		return a->FileName < b->FileName;
	});
	
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
	
	m_drag_payload->set_caption(node->FileName);
	m_drag_payload->set_visible(true);
	m_drag_payload->set_size(m_drag_payload->preferred_size(screen().nvg_context()));
	
	drag_container->add_child(*m_drag_payload);
	drag_container->set_size(m_drag_payload->size());
	drag_container->set_position(screen().mouse_pos() - m_drag_payload->size() / 2);
	drag_container->set_visible(true);
	
	screen().set_drag_widget(drag_container, [this, drag_container, node]() {
		std::vector<std::string> paths = { node->FullPath };
		screen().drop_event(*this, paths);
		
		m_drag_payload->set_visible(false);
		this->add_child(*m_drag_payload);
		this->remove_child(*m_drag_payload);
		
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
