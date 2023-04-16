#include "Environment.h"
#include <vector>
#include <set>
#include <cstdlib>

std::set<std::filesystem::path> gPaths = { std::filesystem::current_path() };
std::filesystem::path gTempFileDir = "./Temp";
std::filesystem::path gDefaultShaderDir = std::filesystem::path(__FILE__).parent_path().parent_path() / "Shaders";

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

bool Environment::findFileRecursively(const std::filesystem::path& vTargetName, const std::filesystem::path& vSearchDirSet, std::filesystem::path& voFilePath)
{
    for (const auto& Entry : std::filesystem::recursive_directory_iterator(vSearchDirSet))
    {
        if (Entry.is_directory())
            continue;

        auto BaseName = Entry.path().filename();
        if (BaseName == vTargetName)
        {
            voFilePath = Entry.path();
            return true;
        }
    }
    return false;
}


std::string Environment::getEnvironmentVariable(std::string vKey)
{
    const char* pResult = std::getenv(vKey.c_str());
    return pResult ? std::string(pResult) : "";
}

std::filesystem::path Environment::normalizePath(const std::filesystem::path& vPath)
{
    return std::filesystem::weakly_canonical(std::filesystem::absolute(vPath));
}

int Environment::execute(const std::string& vCommand, const std::string& vOutputFile)
{
    std::string Command = vCommand + (vOutputFile.empty() ? "" : (" > \"" + vOutputFile + "\""));
    return std::system(Command.c_str());
}

std::vector<std::filesystem::path> __toElementVector(const std::filesystem::path& vDir)
{
    return std::vector<std::filesystem::path>(vDir.begin(), vDir.end());
}

bool __isBelow(const std::filesystem::path& vDir, const std::filesystem::path& vPath)
{
    auto DirElementSet = __toElementVector(Environment::normalizePath(vDir));
    auto PathElementSet = __toElementVector(Environment::normalizePath(vPath));
    
    if (PathElementSet.size() < DirElementSet.size()) return false;
    for (size_t i = 0; i < DirElementSet.size(); ++i)
    {
        if (DirElementSet[i] != PathElementSet[i]) return false;
    }
    return true;
}

std::filesystem::path Environment::getTempFilePath(const std::filesystem::path& vFileName)
{
    _ASSERTE(vFileName.is_relative());
    std::filesystem::path TempFilePath = normalizePath(gTempFileDir / vFileName);
    std::filesystem::path ParentDir = TempFilePath.parent_path();
    if (!std::filesystem::exists(ParentDir))
        std::filesystem::create_directories(ParentDir);
    _ASSERTE(__isBelow(gTempFileDir, TempFilePath));
    return TempFilePath;
}

std::filesystem::path Environment::getShaderDir()
{
    return gDefaultShaderDir;
}

std::filesystem::path Environment::findShader(std::filesystem::path vShaderFileName)
{
    std::filesystem::path FoundShaderPath;
    if (Environment::findFileRecursively(vShaderFileName, getShaderDir(), FoundShaderPath))
    {
        return FoundShaderPath;
    }
    else
        throw std::runtime_error("Error: can not find shader: " + vShaderFileName.string());
}
