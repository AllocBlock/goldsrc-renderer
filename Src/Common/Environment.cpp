#include "Environment.h"
#include <vector>
#include <set>

std::set<std::filesystem::path> gPaths = { std::filesystem::current_path() };

bool __findFileOnTree(const std::filesystem::path& vFileName, const std::filesystem::path& vLeafDir, std::filesystem::path& voFilePath)
{
    _ASSERTE(vFileName.filename() == vFileName);

    std::filesystem::path CurDir = vLeafDir;

    if (!CurDir.empty())
    {
        while (true)
        {
            std::filesystem::path CurPath = CurDir / vFileName;
            if (std::filesystem::exists(CurPath))
            {
                voFilePath = CurPath;
                return true;
            }
            if (CurDir == CurDir.parent_path()) break;

            CurDir = CurDir.parent_path();
        }
    }

    return false;
}

bool Environment::findFile(const std::filesystem::path& vTargetName, bool vSearchInEnvironment, std::filesystem::path& voFilePath)
{
    return findFile(vTargetName, "", vSearchInEnvironment, voFilePath);
}

bool Environment::findFile(const std::filesystem::path& vTargetName, const std::filesystem::path& vAddtionalSearchDir, bool vSearchInEnvironment, std::filesystem::path& voFilePath)
{
    std::filesystem::path FullPath = std::filesystem::absolute(vTargetName);
    std::filesystem::path CurDir = FullPath.parent_path();
    std::filesystem::path FileName = FullPath.filename();

    std::vector<std::filesystem::path> SearchDirs;
    SearchDirs.emplace_back(CurDir); 
    if (!vAddtionalSearchDir.empty()) SearchDirs.emplace_back(vAddtionalSearchDir);
    if (vSearchInEnvironment) SearchDirs.insert(SearchDirs.end(), gPaths.begin(), gPaths.end());

    for (const std::filesystem::path& SearchDir : SearchDirs)
    {
        if (__findFileOnTree(FileName, SearchDir, voFilePath))
        {
            addPathToEnviroment(SearchDir); // add directory to enviroment for next search
            return true;
        }
    }
    return false;
}

void Environment::addPathToEnviroment(std::filesystem::path vPath)
{
    if (!std::filesystem::is_directory(vPath))
        vPath = vPath.parent_path();
    vPath = std::filesystem::absolute(vPath);
    vPath = std::filesystem::canonical(vPath);
	gPaths.insert(vPath);
}