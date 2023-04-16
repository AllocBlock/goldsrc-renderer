#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include <filesystem>

namespace Environment
{
	// search file in filesystem, target name cant be a filename or path, the found file's parent dir will be add to environment for future search
	void addPathToEnviroment(std::filesystem::path vPath);
	bool findFile(const std::filesystem::path& vTargetName, bool vSearchInEnvironment, std::filesystem::path& voFilePath);
	bool findFile(const std::filesystem::path& vTargetName, const std::filesystem::path& vAddtionalSearchDir, bool vSearchInEnvironment, std::filesystem::path& voFilePath);
	bool findFile(const std::filesystem::path& vTargetName, const std::vector<std::filesystem::path>& vAddtionalSearchDirSet, bool vSearchInEnvironment, std::filesystem::path& voFilePath);
	bool findFileRecursively(const std::filesystem::path& vTargetName, const std::filesystem::path& vSearchDirSet, std::filesystem::path& voFilePath);
	
	std::string getEnvironmentVariable(std::string vKey);
	std::filesystem::path normalizePath(const std::filesystem::path& vPath);
    int execute(const std::string& vCommand, const std::string& vOutputFile = "");

	std::filesystem::path getTempFilePath(const std::filesystem::path& vFileName);
	std::filesystem::path getShaderDir();

	std::filesystem::path findShader(std::filesystem::path vShaderFileName);
};
