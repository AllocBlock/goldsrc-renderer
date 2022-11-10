#include "ShaderCompiler.h"
#include "Log.h"
#include "Common.h"
#include "Environment.h"

#include <cstdlib>

bool gInited = false;
std::filesystem::path gCompilePath = "";

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
    Log::log("Брвы [" + vPath.string() + " -> " + OutputPath.string() + "]");
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

std::vector<uint8_t> ShaderCompiler::requestSpirV(const std::filesystem::path& vPath)
{
    if (!gInited) __init();

    std::filesystem::path BinPath = compile(vPath);
    auto Data = Common::readFileAsByte(BinPath);
    return Data;
}
