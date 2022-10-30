#include "Environment.h"
#include <vector>
#include <set>

std::set<std::filesystem::path> gPaths = { std::filesystem::current_path() };

std::filesystem::path __cleanAbsolutePrefix(const std::filesystem::path& vPath)
{
    auto AbsPath = std::filesystem::absolute(vPath);
    auto RootDir = vPath.root_directory();
    return std::filesystem::relative(AbsPath, RootDir);
}

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
    const std::filesystem::path TargetName = vTargetName.filename();
    const std::filesystem::path TargetRelativePath = __cleanAbsolutePrefix(vTargetName);

    const std::filesystem::path FullPath = std::filesystem::absolute(vTargetName);
    const std::filesystem::path InputFileDir = FullPath.parent_path();

    std::set<std::filesystem::path> SearchDirs;
    SearchDirs.insert(InputFileDir); 
    if (!vAddtionalSearchDir.empty())
    {
        std::filesystem::path AddDir = std::filesystem::absolute(vAddtionalSearchDir / TargetRelativePath).parent_path();
        SearchDirs.insert(AddDir);
    }
    if (vSearchInEnvironment)
        for (const auto& Path : gPaths)
            SearchDirs.insert(Path);

    for (const std::filesystem::path& SearchDir : SearchDirs)
    {
        if (__findFileOnTree(TargetName, SearchDir, voFilePath))
        {
            addPathToEnviroment(voFilePath.parent_path()); // add directory to environment for next search
            return true;
        }
    }
    return false;
}

bool Environment::findFile(const std::filesystem::path& vTargetName, const std::vector<std::filesystem::path>& vAddtionalSearchDirSet, bool vSearchInEnvironment, std::filesystem::path& voFilePath)
{
    for (const auto& SearchDir : vAddtionalSearchDirSet)
        if (Environment::findFile(vTargetName, SearchDir, vSearchInEnvironment, voFilePath))
            return true;
    return false;
}

void Environment::addPathToEnviroment(std::filesystem::path vPath)
{
    if (std::filesystem::exists(vPath))
    {
        if (!std::filesystem::is_directory(vPath))
            vPath = vPath.parent_path();
        vPath = std::filesystem::absolute(vPath);
        vPath = std::filesystem::canonical(vPath);
    } // if not exist, it maybe a virtual path

	gPaths.insert(vPath);
}