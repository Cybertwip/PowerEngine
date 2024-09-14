#pragma once

#include <filesystem>
#include <memory>

struct DirectoryNode
{
	static std::unique_ptr<DirectoryNode> create(const std::string& path);
	
	void refresh();
	
	std::string FullPath;
	std::string FileName;
	std::vector<std::unique_ptr<DirectoryNode>> Children;
	bool IsDirectory;
};

namespace {

void RecursivelyAddDirectoryNodes(DirectoryNode& parentNode, std::filesystem::directory_iterator directoryIterator)
{
	for (const std::filesystem::directory_entry& entry : directoryIterator)
	{
		
		if(entry.is_directory()){
			auto node = std::make_unique<DirectoryNode>();
			DirectoryNode& childNode = *node;
			parentNode.Children.push_back(std::move(node));
			childNode.FullPath = entry.path().string();
			childNode.FileName = entry.path().filename().string();
			if (childNode.IsDirectory = entry.is_directory(); childNode.IsDirectory){
				RecursivelyAddDirectoryNodes(childNode, std::filesystem::directory_iterator(entry));
			}
		} else {
			if (entry.is_regular_file() && (entry.path().extension() == ".fbx" || entry.path().extension() == ".seq" ||
											entry.path().extension() == ".pwr" || entry.path().extension() == ".cmp")) {
				auto node = std::make_unique<DirectoryNode>();
				DirectoryNode& childNode = *node;
				parentNode.Children.push_back(std::move(node));
				childNode.FullPath = entry.path().string();
				childNode.FileName = entry.path().filename().string();
			}
			
		}
	}
	
	auto moveDirectoriesToFront = [](const std::unique_ptr<DirectoryNode>& a, const std::unique_ptr<DirectoryNode>& b) { return (a->IsDirectory > b->IsDirectory); };
	std::sort(parentNode.Children.begin(), parentNode.Children.end(), moveDirectoriesToFront);
}

std::unique_ptr<DirectoryNode> CreateDirectoryNodeTreeFromPath(const std::filesystem::path& rootPath)
{
	if (!std::filesystem::exists(rootPath) || !std::filesystem::is_directory(rootPath)) {
		std::filesystem::create_directories(rootPath);
	}
	
	auto rootNode = std::make_unique<DirectoryNode>();
	rootNode->FullPath = rootPath.string();
	rootNode->FileName = rootPath.filename().string();
	if (rootNode->IsDirectory = std::filesystem::is_directory(rootPath); rootNode->IsDirectory)
		RecursivelyAddDirectoryNodes(*rootNode, std::filesystem::directory_iterator(rootPath));
	
	return std::move(rootNode);
}

void RecursivelyCleanDirectoryNodes(DirectoryNode& parentNode)
{
	auto it = std::remove_if(parentNode.Children.begin(), parentNode.Children.end(),
							 [](std::unique_ptr<DirectoryNode>& childNode)
							 {
		
		if(!childNode->IsDirectory){
			return false;
		}
		
		bool hasFile = false;
		
		for(auto& entry : childNode->Children){
			if(!entry->IsDirectory){
				hasFile = true;
			}
		}
		
		if (childNode->Children.empty() || !hasFile)
		{
			return true; // Mark for removal
		}
		else
		{
			RecursivelyCleanDirectoryNodes(*childNode);
			return false;
		}
	});
	
	if (it != parentNode.Children.end()) {
		parentNode.Children.erase(it, parentNode.Children.end());
	}
}
}

