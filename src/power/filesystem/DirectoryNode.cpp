#include "DirectoryNode.hpp"


std::unique_ptr<DirectoryNode> DirectoryNode::create(const std::string& path) {
	// Ensure the directory exists before creating the node tree
	if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
		return nullptr; // Return nullptr if path is invalid or not a directory
	}
	
	// Create the root node and populate the tree
	auto rootNode = std::make_unique<DirectoryNode>();
	rootNode->FullPath = path;
	rootNode->FileName = std::filesystem::path(path).filename().string();
	
	rootNode->refresh(); // Refresh the node to build its tree structure
	
	return std::move(rootNode);
}

bool DirectoryNode::refresh(const std::set<std::string>& allowedExtensions) {
	// Clear all existing children to rebuild the tree from scratch
	this->Children.clear();
	
	bool hasValidChildren = false; // Track if there are any valid children (files or directories)
	
	// Check if the current node's path still exists and is a directory
	if (std::filesystem::exists(this->FullPath) && std::filesystem::is_directory(this->FullPath)) {
		// Iterate through the directory again to rebuild the tree
		for (const auto& entry : std::filesystem::directory_iterator(this->FullPath)) {
			// If it's a directory, refresh its contents recursively
			if (entry.is_directory()) {
				auto newNode = std::make_unique<DirectoryNode>();
				newNode->FullPath = entry.path().string();
				newNode->FileName = entry.path().filename().string();
				newNode->IsDirectory = true;
				
				// Recursively refresh child directories, and only add them if they contain valid files
				if (newNode->refresh(allowedExtensions)) {
					this->Children.push_back(std::move(newNode));
					hasValidChildren = true; // Mark this node as having valid children
				}
			}
			// If it's a regular file, filter it by allowed extensions
			else if (entry.is_regular_file() && allowedExtensions.count(entry.path().extension().string()) > 0) {
				auto newNode = std::make_unique<DirectoryNode>();
				newNode->FullPath = entry.path().string();
				newNode->FileName = entry.path().filename().string();
				newNode->IsDirectory = false;
				
				// Add the file node to the children of the current node
				this->Children.push_back(std::move(newNode));
				hasValidChildren = true; // Mark this node as having valid children
			}
		}
	} else {
		// If the path no longer exists or isn't a directory, handle this situation
		this->IsDirectory = false; // Mark as not a directory or could set some invalid flag
	}
	
	// Return true if this node has valid children (files or directories), false otherwise
	return hasValidChildren;
}
