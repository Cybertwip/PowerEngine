#pragma once

#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <iostream>

struct DirectoryNode : public std::enable_shared_from_this<DirectoryNode> {
public:
	static std::shared_ptr<DirectoryNode> initialize(const std::string& path) {
		static std::shared_ptr<DirectoryNode> instance = nullptr;
		
		if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
			if (!instance) {
				// Use `new`. The shared_ptr constructor will manage the memory.
				// This is allowed because a static member can access private constructors.
				instance = std::shared_ptr<DirectoryNode>(new DirectoryNode(""));
			}
			return instance;
		}
		
		if (!instance || instance->FullPath != path) {
			// Same fix here: use `new` directly.
			instance = std::shared_ptr<DirectoryNode>(new DirectoryNode(path));
		}
		
		instance->refresh();
		return instance;
	}
	
	bool refresh(const std::set<std::string>& allowedExtensions = {".psk", ".pma", ".pan", ".psq", ".psn", ".png"}) {
		if (!IsDirectory || !std::filesystem::exists(FullPath)) {
			return false;
		}
		
		Children.clear();
		bool hasValidChildren = false;
		
		try {
			for (const auto& entry : std::filesystem::directory_iterator(FullPath)) {
				// A regular member function can also access private constructors.
				auto newNode = std::shared_ptr<DirectoryNode>(new DirectoryNode(entry.path().string(), shared_from_this()));
				
				if (newNode->IsDirectory) {
					if (newNode->refresh(allowedExtensions)) {
						Children.push_back(newNode);
						hasValidChildren = true;
					}
				} else if (entry.is_regular_file()) {
					const std::string extension = entry.path().extension().string();
					if (allowedExtensions.empty() || allowedExtensions.count(extension) > 0) {
						Children.push_back(newNode);
						hasValidChildren = true;
					}
				}
			}
		} catch (const std::filesystem::filesystem_error& e) {
			std::cerr << "Filesystem error accessing " << FullPath << ": " << e.what() << std::endl;
		}
		
		return hasValidChildren;
	}
	
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
	std::weak_ptr<DirectoryNode> Parent;
	
private:
	// Constructors are private again, which was the original intent.
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
	
	// Disallow copy/move.
	DirectoryNode(const DirectoryNode&) = delete;
	DirectoryNode& operator=(const DirectoryNode&) = delete;
	DirectoryNode(DirectoryNode&&) = delete;
	DirectoryNode& operator=(DirectoryNode&&) = delete;
};
