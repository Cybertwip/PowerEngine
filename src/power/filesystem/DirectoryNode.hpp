#pragma once

#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <vector>

struct DirectoryNode : public std::enable_shared_from_this<DirectoryNode> {
	// Singleton instance
	static DirectoryNode& getInstance() {
		static DirectoryNode instance;
		return instance;
	}
	
	// Prevent copying and moving
	DirectoryNode(const DirectoryNode&) = delete;
	DirectoryNode& operator=(const DirectoryNode&) = delete;
	DirectoryNode(DirectoryNode&&) = delete;
	DirectoryNode& operator=(DirectoryNode&&) = delete;
	
	// Initialize or update the singleton with a new directory path
	static bool initialize(const std::string& path) {
		if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
			return false; // Return false if path is invalid or not a directory
		}
		
		DirectoryNode& instance = getInstance();
		instance.FullPath = path;
		instance.FileName = std::filesystem::path(path).filename().string();
		instance.IsDirectory = true;
		instance.Children.clear(); // Clear existing children before refreshing
		return instance.refresh(); // Refresh the node to build its tree structure
	}
	
	bool refresh(const std::set<std::string>& allowedExtensions = {".psk", ".pma", ".pan", ".psq", ".psn", ".png"}) {
		// Clear all existing children to rebuild the tree from scratch
		Children.clear();
		
		bool hasValidChildren = false; // Track if there are any valid children
		
		// Check if the current node's path still exists and is a directory
		if (!std::filesystem::exists(FullPath) || !std::filesystem::is_directory(FullPath)) {
			IsDirectory = false;
			return false;
		}
		
		// Iterate through the directory to rebuild the tree
		for (const auto& entry : std::filesystem::directory_iterator(FullPath)) {
			// Create a new node for each entry
			auto newNode = std::shared_ptr<DirectoryNode>(new DirectoryNode(entry.path().string()));
			
			if (entry.is_directory()) {
				newNode->IsDirectory = true;
				// Recursively refresh child directories, and only add them if they contain valid files
				if (newNode->refresh(allowedExtensions)) {
					Children.push_back(newNode);
					hasValidChildren = true;
				}
			} else if (entry.is_regular_file() && allowedExtensions.count(entry.path().extension().string()) > 0) {
				newNode->IsDirectory = false;
				Children.push_back(newNode);
				hasValidChildren = true;
			}
		}
		
		return hasValidChildren;
	}
	
	// Method to create a new project folder and assign it to the singleton
	static bool createProjectFolder(const std::string& folderPath) {
		try {
			// Create the directory if it doesn't exist
			if (!std::filesystem::exists(folderPath)) {
				std::filesystem::create_directories(folderPath);
			}
			// Initialize the singleton with the new folder path
			return initialize(folderPath);
		} catch (const std::filesystem::filesystem_error&) {
			return false;
		}
	}
	
	std::string FullPath;
	std::string FileName;
	std::vector<std::shared_ptr<DirectoryNode>> Children;
	bool IsDirectory;
	
private:
	// Private constructor for singleton and node creation
	explicit DirectoryNode(const std::string& path = "")
	: FullPath(path), FileName(std::filesystem::path(path).filename().string()), IsDirectory(false) {}
	
	// Allow shared_ptr construction
	friend class std::enable_shared_from_this<DirectoryNode>;
};
