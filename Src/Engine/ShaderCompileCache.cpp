#include "ShaderCompileCache.h"
#include "Common.h"
#include "Environment.h"

#include <chrono>
#include <sstream>
#include <fstream>

using namespace std::chrono;

const std::string gTimeStringFormat = "%Y-%m-%d_%H:%M:%S";

std::string __toTimeString(system_clock::time_point vTimestamp)
{
    auto TimeT = system_clock::to_time_t(vTimestamp);
    std::tm* Gmt = std::gmtime(&TimeT);
    std::stringstream Buffer;
    Buffer << std::put_time(Gmt, gTimeStringFormat.c_str());
    return Buffer.str();
}

std::string __getChangeTime(const std::filesystem::path& vPath)
{
    auto FileTime = std::filesystem::last_write_time(vPath);
    auto SystemClockTime = time_point_cast<system_clock::duration>(FileTime - std::filesystem::file_time_type::clock::now()
        + system_clock::now());
    return __toTimeString(SystemClockTime);
}

void CShaderCompileCache::load(const std::filesystem::path& vPath)
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

void CShaderCompileCache::save(const std::filesystem::path& vPath)
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

void CShaderCompileCache::add(const std::filesystem::path& vSourcePath, const std::filesystem::path& vBinPath)
{
    auto SrcPath = Environment::normalizePath(vSourcePath);
    m_CacheMap[SrcPath] = { Environment::normalizePath(vBinPath), __getChangeTime(SrcPath) };
}

bool CShaderCompileCache::doesNeedRecompile(const std::filesystem::path& vPath)
{
    auto Path = Environment::normalizePath(vPath);
    if (m_CacheMap.find(Path) == m_CacheMap.end()) return true; // no record, need recompile
    
    const SShaderCompileInfo& Record = m_CacheMap.at(Path);
    auto RecordTime = Record.ChangeTime;
    auto CurTime = __getChangeTime(Path);
    if (RecordTime != CurTime) return true; // changed since last record, need recompile

    if (!std::filesystem::exists(Record.BinPath)) return true; // bin file not exist, need recompile

    return false;
}

std::filesystem::path CShaderCompileCache::getBinPath(const std::filesystem::path& vSourcePath)
{
    _ASSERTE(m_CacheMap.find(vSourcePath) != m_CacheMap.end());
    return m_CacheMap.at(vSourcePath).BinPath;
}