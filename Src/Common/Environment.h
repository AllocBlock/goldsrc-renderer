#pragma once
#include <filesystem>
namespace Environment
{
	bool findFile(const std::filesystem::path& vTargetName, bool vSearchInEnvironment, std::filesystem::path& voFilePath);
	bool findFile(const std::filesystem::path& vTargetName, const std::filesystem::path& vAddtionalSearchDir, bool vSearchInEnvironment, std::filesystem::path& voFilePath);
	void addPathToEnviroment(std::filesystem::path vPath);
};

