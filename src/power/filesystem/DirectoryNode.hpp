#pragma once

#include <filesystem>
#include <memory>

struct DirectoryNode
{
	static std::unique_ptr<DirectoryNode> create(const std::string& path);
	
	bool refresh(const std::set<std::string>& allowedExtensions = {".fbx", ".seq", ".pwr", ".cmp"});
	
	std::string FullPath;
	std::string FileName;
	std::vector<std::unique_ptr<DirectoryNode>> Children;
	bool IsDirectory;
};
