#pragma once

#include <filesystem>
#include <memory>
#include <set>

struct DirectoryNode : public std::enable_shared_from_this<DirectoryNode>
{
	static std::unique_ptr<DirectoryNode> create(const std::string& path);
	
	bool refresh(const std::set<std::string>& allowedExtensions = {".psk", ".pma", ".pan", ".psq", ".psn", ".png"});
	
	std::string FullPath;
	std::string FileName;
	std::vector<std::shared_ptr<DirectoryNode>> Children;
	bool IsDirectory;
};
