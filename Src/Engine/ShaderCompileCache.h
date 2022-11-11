#pragma once
#include <filesystem>
#include <map>

class CShaderCompileCache
{
public:
    void load(const std::filesystem::path& vPath);
    void save(const std::filesystem::path& vPath);
    void add(const std::filesystem::path& vSourcePath, const std::filesystem::path& vBinPath);
    bool doesNeedRecompile(const std::filesystem::path& vPath);
    std::filesystem::path getBinPath(const std::filesystem::path& vSourcePath);

private:
    struct SShaderCompileInfo
    {
        std::filesystem::path BinPath;
        std::string ChangeTime; // accurate to second
    };

    std::map<std::filesystem::path, SShaderCompileInfo> m_CacheMap;
};
