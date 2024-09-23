#pragma once

#include <filesystem>
#include <memory>
#include <set>

struct DirectoryNode
{
	static std::unique_ptr<DirectoryNode> create(const std::string& path);
	
	bool refresh(const std::set<std::string>& allowedExtensions = {".fbx", ".seq", ".pwr", ".cmp"});
	
	std::string FullPath;
	std::string FileName;
	std::vector<std::shared_ptr<DirectoryNode>> Children;
	bool IsDirectory;
};
