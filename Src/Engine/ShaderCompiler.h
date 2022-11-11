#pragma once
#include <filesystem>

// compile shader
namespace ShaderCompiler
{
    void addHeaderDir(const std::filesystem::path& vPath);
    std::filesystem::path compile(const std::filesystem::path& vPath);
    std::vector<uint8_t> requestSpirV(const std::filesystem::path& vPath, bool vEnableCache = true);
};
