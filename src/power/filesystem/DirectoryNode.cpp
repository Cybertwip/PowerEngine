#include "DirectoryNode.hpp"

std::unique_ptr<DirectoryNode> DirectoryNode::create(const std::string& path) {
	auto rootNode = CreateDirectoryNodeTreeFromPath(std::filesystem::current_path().string());
	
	//RecursivelyCleanDirectoryNodes(*rootNode);
	
	return std::move(rootNode);
}

void DirectoryNode::refresh() {
	// Clear all existing children to rebuild the tree from scratch
	this->Children.clear();
	
	// Check if the current node's path still exists and is a directory
	if (std::filesystem::exists(this->FullPath) && std::filesystem::is_directory(this->FullPath)) {
		// Iterate through the directory again to rebuild the tree
		for (const auto &entry : std::filesystem::directory_iterator(this->FullPath)) {
			auto newNode = std::make_unique<DirectoryNode>();
			newNode->FullPath = entry.path().string();
			newNode->FileName = entry.path().filename().string();
			newNode->IsDirectory = entry.is_directory();
			
			// If it's a directory, we might want to recursively refresh or at least mark for potential expansion
			if (newNode->IsDirectory) {
				// Optionally, you could call refresh on this new node if you want to fully expand all directories
				newNode->refresh();
				// For now, we'll just add it as is, assuming it might be expanded later
			}
			
			// Add the new node to the children of the current node
			this->Children.push_back(std::move(newNode));
		}
	} else {
		// If the path no longer exists or isn't a directory, handle this situation
		// Perhaps by marking the node as invalid or removing it if it's part of a larger tree
		this->IsDirectory = false; // Mark as not a directory or could set some invalid flag
	}
	
	// After refreshing, clean up any nodes that might no longer be valid or needed
	//RecursivelyCleanDirectoryNodes(*this);
}
