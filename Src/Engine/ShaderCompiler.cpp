#include "ShaderCompiler.h"
#include "Log.h"
#include "Common.h"
#include "Environment.h"

#include <cstdlib>
#include <map>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>

std::filesystem::path gCompileCacheFile = "shaderCompileCache.txt";

using namespace std::chrono;

class CShaderCompileCache
{
public:
    struct SShaderCompileInfo
    {
        std::filesystem::path BinPath;
        std::string ChangeTime; // accurate to second
    };

    void load(const std::filesystem::path& vPath)
    {
        if (!std::filesystem::exists(vPath))
            throw std::runtime_error("File not exist");

        m_CacheMap.clear();

        std::string DataStr = Common::readFileAsString(vPath);

        auto LineSet = Common::split(DataStr, "\n");
        for (const auto& Line : LineSet)
        {
            if (Line.empty()) continue;
            auto FieldSet = Common::split(Line, "\t");
            _ASSERTE(FieldSet.size() == 3);
            const std::string& SrcPath = FieldSet[0];
            const std::string& BinPath = FieldSet[1];
            const std::string& ChangeTime = FieldSet[2];
            m_CacheMap[SrcPath] = { BinPath, ChangeTime };
        }
    }

    void save(const std::filesystem::path& vPath)
    {
        std::ofstream File(vPath);
        for (auto Pair : m_CacheMap)
        {
            File << Pair.first.string() << "\t";
            File << Pair.second.BinPath.string() << "\t";
            File << Pair.second.ChangeTime << "\n";
        }
        File.close();
    }

    void add(const std::filesystem::path& vSourcePath, const std::filesystem::path& vBinPath)
    {
        auto SrcPath = __normalizePath(vSourcePath);
        m_CacheMap[SrcPath] = { __normalizePath(vBinPath), __getChangeTime(SrcPath) };
    }

    bool doesNeedRecompile(const std::filesystem::path& vPath)
    {
        auto Path = __normalizePath(vPath);
        if (m_CacheMap.find(Path) == m_CacheMap.end()) return true; // no record, need recompile

        auto RecordTime = m_CacheMap.at(Path).ChangeTime;
        auto CurTime = __getChangeTime(Path);
        if (RecordTime != CurTime) return true; // changed since last record, need recompile

        return false;
    }

    std::filesystem::path getBinPath(const std::filesystem::path& vSourcePath)
    {
        _ASSERTE(m_CacheMap.find(vSourcePath) != m_CacheMap.end());
        return m_CacheMap.at(vSourcePath).BinPath;
    }

private:
    static std::filesystem::path __normalizePath(const std::filesystem::path& vPath)
    {
        return std::filesystem::weakly_canonical(std::filesystem::absolute(vPath));
    }

    std::string __getChangeTime(const std::filesystem::path& vPath) const
    {
        auto FileTime = std::filesystem::last_write_time(vPath);
        auto SystemClockTime = time_point_cast<system_clock::duration>(FileTime - std::filesystem::file_time_type::clock::now()
            + system_clock::now());
        return __toTimeString(SystemClockTime);
    }

    std::string __toTimeString(system_clock::time_point vTimestamp) const
    {
        auto TimeT = system_clock::to_time_t(vTimestamp);
        std::tm* Gmt = std::gmtime(&TimeT);
        std::stringstream Buffer;
        Buffer << std::put_time(Gmt, m_TimeStringFormat.c_str());
        return Buffer.str();
    }

    std::map<std::filesystem::path, SShaderCompileInfo> m_CacheMap;
    const std::string m_TimeStringFormat = "%Y-%m-%d_%H:%M:%S"; 
};

bool gInited = false;
std::filesystem::path gCompilePath = "";
CShaderCompileCache gCompileCache = CShaderCompileCache();

std::string __getEnvironmentVariable(std::string vKey)
{
    const char* pResult = std::getenv(vKey.c_str());
    return pResult ? std::string(pResult) : "";
}

void __init()
{
    _ASSERTE(!gInited);
    // find compiler path from environment variable
    std::vector<std::string> KeySet = { "VK_SDK_PATH", "VULKAN_SDK" };
    for (const auto& Key : KeySet)
    {
        auto Val = __getEnvironmentVariable(Key);
        if (!Val.empty())
        {
            auto ExePath = std::filesystem::path(Val) / "Bin/glslangValidator.exe";
            if (std::filesystem::exists(ExePath))
            {
                gCompilePath = std::filesystem::canonical(ExePath);
                break;
            }
        }
    }

    if (gCompilePath.empty())
        throw std::runtime_error("Vulkan compiler not found, please check Vulkan SDK environment variable (VK_SDK_PATH, VULKAN_SDK)");

    if (std::filesystem::exists(gCompileCacheFile))
        gCompileCache.load(gCompileCacheFile);
    gInited = true;
}

std::filesystem::path __generateCompiledShaderPath(const std::filesystem::path& vPath)
{
    auto Ext = vPath.extension().string().substr(1);
    auto Postfix = char(std::toupper(Ext[0])) + Ext.substr(1);
    auto NewPath = vPath;
    NewPath.replace_extension("");
    NewPath += Postfix + ".spv";
    return NewPath;
}

std::string gTempOutputFile = "Result.temp";

std::string __execute(std::string vCommand)
{
    std::system((vCommand + " > " + gTempOutputFile).c_str());
    return Common::readFileAsString(gTempOutputFile);
}

std::filesystem::path ShaderCompiler::compile(const std::filesystem::path& vPath)
{
    if (!gInited) __init();
    
    std::filesystem::path ActualPath;
    if (!Environment::findFile(vPath, true, ActualPath))
        throw std::runtime_error("File not found");

    auto OutputPath = __generateCompiledShaderPath(vPath);
    std::string Cmd = gCompilePath.string() + " -V \"" + vPath.string() + "\" -o \"" + OutputPath.string() + "\"";
    auto Result = __execute(Cmd);

    // check if error
    if (Result.find("Error") != std::string::npos)
    {
        Log::log("Compile failed on file " + vPath.string());
        Log::log(Result);
        throw std::runtime_error("Compile failed");
        return "";
    }
    
    return OutputPath;
}

std::vector<uint8_t> ShaderCompiler::requestSpirV(const std::filesystem::path& vPath, bool vEnableCache)
{
    if (!gInited) __init();

    std::filesystem::path BinPath;
    if (!vEnableCache || gCompileCache.doesNeedRecompile(vPath))
    {
        Log::log("编译 [" + vPath.string() + "]");
        BinPath = compile(vPath);
        Log::log("编译完成 [" + vPath.filename().string() + " -> " + BinPath.string() + "]");
        gCompileCache.add(vPath, BinPath);
        gCompileCache.save(gCompileCacheFile);
    }
    else
    {
        Log::log("无需编译 [" + vPath.string() + "]");
        BinPath = gCompileCache.getBinPath(vPath);
    }
    
    auto Data = Common::readFileAsByte(BinPath);
    return Data;
}
