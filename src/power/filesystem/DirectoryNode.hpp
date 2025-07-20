#pragma once

#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <iostream> // For logging potential errors

// The DirectoryNode represents a file or folder in the filesystem.
// It uses a tree structure with shared_ptr for children and weak_ptr for the parent
// to manage memory automatically and avoid circular references.
struct DirectoryNode : public std::enable_shared_from_this<DirectoryNode> {
public:
	// This static method initializes or retrieves the root of the directory tree.
	// It's a simplified singleton pattern returning a shared_ptr.
	static std::shared_ptr<DirectoryNode> initialize(const std::string& path) {
		static std::shared_ptr<DirectoryNode> instance = nullptr;
		
		if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
			// Path is invalid. If we've never created an instance, make an empty one.
			if (!instance) {
				instance = std::make_shared<DirectoryNode>("");
			}
			return instance;
		}
		
		// If the path is different from the current instance, create a new root.
		if (!instance || instance->FullPath != path) {
			instance = std::make_shared<DirectoryNode>(path);
		}
		
		// Always refresh the contents from the filesystem to ensure they are up-to-date.
		instance->refresh();
		return instance;
	}
	
	// Refreshes the children of this node from the filesystem.
	// It recursively builds a tree, but prunes branches (subdirectories) that
	// do not contain any files matching the allowed extensions.
	// Returns true if the directory contains any valid children.
	bool refresh(const std::set<std::string>& allowedExtensions = {".psk", ".pma", ".pan", ".psq", ".psn", ".png"}) {
		// We can only refresh nodes that are actual directories.
		if (!IsDirectory || !std::filesystem::exists(FullPath)) {
			return false;
		}
		
		Children.clear();
		bool hasValidChildren = false;
		
		try {
			// Using a directory_iterator can throw an exception (e.g., permissions denied).
			for (const auto& entry : std::filesystem::directory_iterator(FullPath)) {
				// Use make_shared to create the child node. Pass `shared_from_this()` to correctly
				// set up the weak_ptr for the parent.
				auto newNode = std::make_shared<DirectoryNode>(entry.path().string(), shared_from_this());
				
				if (newNode->IsDirectory) {
					// Recursively refresh the subdirectory. If it has valid content (returns true),
					// then we add it to our list of children.
					if (newNode->refresh(allowedExtensions)) {
						Children.push_back(newNode);
						hasValidChildren = true;
					}
				} else if (entry.is_regular_file()) {
					// For files, check if the extension is in our allowed set.
					const std::string extension = entry.path().extension().string();
					if (allowedExtensions.empty() || allowedExtensions.count(extension) > 0) {
						Children.push_back(newNode);
						hasValidChildren = true;
					}
				}
			}
		} catch (const std::filesystem::filesystem_error& e) {
			// It's common to encounter directories you can't read. Log the error instead of crashing.
			std::cerr << "Filesystem error accessing " << FullPath << ": " << e.what() << std::endl;
		}
		
		return hasValidChildren;
	}
	
	// Creates a new folder and initializes it as the new application root.
	static std::shared_ptr<DirectoryNode> createProjectFolder(const std::string& folderPath) {
		try {
			if (!std::filesystem::exists(folderPath)) {
				std::filesystem::create_directories(folderPath);
			}
			return initialize(folderPath);
		} catch (const std::filesystem::filesystem_error& e) {
			std::cerr << "Failed to create project folder " << folderPath << ": " << e.what() << std::endl;
			return nullptr;
		}
	}
	
	std::string FullPath;
	std::string FileName;
	std::vector<std::shared_ptr<DirectoryNode>> Children;
	bool IsDirectory;
	// A weak_ptr to the parent is crucial. It allows navigating up the tree
	// without creating a circular reference with the parent's `Children` shared_ptr,
	// which would cause a memory leak.
	std::weak_ptr<DirectoryNode> Parent;
	
private:
	// Private constructor for use with make_shared.
	// A node without a parent is a root.
	explicit DirectoryNode(const std::string& path, std::shared_ptr<DirectoryNode> parent = nullptr)
	: FullPath(path), Parent(parent) {
		if (!path.empty()) {
			try {
				FileName = std::filesystem::path(path).filename().string();
				IsDirectory = std::filesystem::is_directory(path);
			} catch (const std::filesystem::filesystem_error& e) {
				FileName = "access_error";
				IsDirectory = false;
				std::cerr << "Error processing path " << path << ": " << e.what() << std::endl;
			}
		} else {
			FileName = "";
			IsDirectory = false;
		}
	}
	
	// Private constructor for the root node, which has no parent.
	explicit DirectoryNode(const std::string& path) : DirectoryNode(path, nullptr) {}
	
	// Disallow copy/move to prevent accidental slicing or breaking of the tree structure.
	DirectoryNode(const DirectoryNode&) = delete;
	DirectoryNode& operator=(const DirectoryNode&) = delete;
	DirectoryNode(DirectoryNode&&) = delete;
	DirectoryNode& operator=(DirectoryNode&&) = delete;
};
