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
	
	// Initialize or update the singleton with a new directory path
	static bool initialize(const std::string& path) {
		if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
			return false; // Return false if path is invalid or not a directory
		}
		
		DirectoryNode& instance = getInstance();
		instance.FullPath = path;
		instance.FileName = std::filesystem::path(path).filename().string();
		instance.IsDirectory = true;
		instance.Parent = nullptr; // Root has no parent
		instance.Children.clear(); // Clear existing children before refreshing
		return instance.refresh(); // Refresh the node to build its tree structure
	}
	
	bool refresh(const std::set<std::string>& allowedExtensions = {".psk", ".pma", ".pan", ".psq", ".psn", ".png"}) {
		Children.clear();
		
		if (!std::filesystem::exists(FullPath) || !std::filesystem::is_directory(FullPath)) {
			IsDirectory = false;
			return false;
		}
		
		// This flag is used in the original code to only show directories that contain valid files.
		// We will keep this logic.
		bool hasValidChildren = false;
		
		for (const auto& entry : std::filesystem::directory_iterator(FullPath)) {
			auto newNode = std::shared_ptr<DirectoryNode>(new DirectoryNode(entry.path().string()));
			newNode->Parent = this; // MODIFIED: Set the parent pointer
			
			if (entry.is_directory()) {
				newNode->IsDirectory = true;
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
			if (!std::filesystem::exists(folderPath)) {
				std::filesystem::create_directories(folderPath);
			}
			return initialize(folderPath);
		} catch (const std::filesystem::filesystem_error&) {
			return false;
		}
	}
	
	std::string FullPath;
	std::string FileName;
	std::vector<std::shared_ptr<DirectoryNode>> Children;
	bool IsDirectory;
	DirectoryNode* Parent; // ADDED: Pointer to the parent node for navigation.
	
private:
	// Private constructor for singleton and node creation
	explicit DirectoryNode(const std::string& path = "")
	: FullPath(path), FileName(std::filesystem::path(path).filename().string()), IsDirectory(false), Parent(nullptr) {}
	
	// Allow shared_ptr construction
	friend class std::enable_shared_from_this<DirectoryNode>;
};
