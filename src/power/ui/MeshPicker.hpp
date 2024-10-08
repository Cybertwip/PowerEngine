#pragma once

#include <nanogui/window.h>
#include <nanogui/textbox.h>
#include <functional>
#include <vector>
#include <string>

// Forward declaration
struct DirectoryNode;

class MeshPicker : public nanogui::Window {
public:
	/**
	 * @brief Constructs the MeshPicker widget.
	 *
	 * @param parent Pointer to the parent widget.
	 * @param root_directory_node Reference to the root DirectoryNode.
	 * @param on_model_selected Callback function triggered on model selection.
	 */
	MeshPicker(nanogui::Screen& parent, nanogui::Screen& screen, DirectoryNode& root_directory_node,
			   std::function<void(const std::string&)> on_model_selected);
	
private:	
	DirectoryNode& root_directory_node_;
	std::function<void(const std::string&)> on_model_selected_;
	
	std::shared_ptr<nanogui::TextBox> filter_box_;
	std::shared_ptr<nanogui::Widget> file_list_widget_;
	
	std::vector<std::string> model_files_;
	
	/**
	 * @brief Recursively searches for model files in the directory tree.
	 *
	 * @param node Current DirectoryNode.
	 */
	void search_model_files(const DirectoryNode& node);
	
	/**
	 * @brief Refreshes the displayed file list based on the current filter.
	 */
	void refresh_file_list();
	
	/**
	 * @brief Handles the double-click event on a file item.
	 *
	 * @param model_path Path of the selected model.
	 */
	void handle_double_click(const std::string& model_path);
	
	std::string mFilterValue;
	
	std::shared_ptr<nanogui::VScrollPanel> mScrollPanel;
	
	std::shared_ptr<nanogui::Button> mCloseButton;
};
