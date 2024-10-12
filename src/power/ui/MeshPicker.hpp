#pragma once

#include <nanogui/window.h>
#include <nanogui/textbox.h>
#include <functional>
#include <vector>
#include <string>

// Forward declaration
struct DirectoryNode;

class FIleView;

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
	void refresh_file_list();
	
	DirectoryNode& root_directory_node_;
	std::function<void(const std::string&)> on_model_selected_;
	
	std::shared_ptr<nanogui::TextBox> filter_box_;
	std::shared_ptr<FIleView> mFileView;
	
	std::string mFilterValue;
		
	std::shared_ptr<nanogui::Button> mCloseButton;
};
