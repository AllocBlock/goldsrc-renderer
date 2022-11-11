#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include <filesystem>

namespace Environment
{
	// search file in filesystem, target name cant be a filename or path, the found file's parent dir will be add to environment for future search
	bool findFile(const std::filesystem::path& vTargetName, bool vSearchInEnvironment, std::filesystem::path& voFilePath);
	bool findFile(const std::filesystem::path& vTargetName, const std::filesystem::path& vAddtionalSearchDir, bool vSearchInEnvironment, std::filesystem::path& voFilePath);
	bool findFile(const std::filesystem::path& vTargetName, const std::vector<std::filesystem::path>& vAddtionalSearchDirSet, bool vSearchInEnvironment, std::filesystem::path& voFilePath);
	void addPathToEnviroment(std::filesystem::path vPath);
	
	std::string getEnvironmentVariable(std::string vKey);

	int execute(const std::string& vCommand, const std::string& vOutputFile = "");
};
