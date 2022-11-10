#pragma once
#include <filesystem>

// compile shader
namespace ShaderCompiler
{
    std::filesystem::path compile(const std::filesystem::path& vPath);
    std::vector<uint8_t> requestSpirV(const std::filesystem::path& vPath);
};