#include "ShaderCompiler.h"
#include "Log.h"
#include "Common.h"
#include "Environment.h"
#include "ShaderCompileCache.h"
#include "ShaderErrorParser.h"

#include <chrono>
#include <sstream>

bool gInited = false;
std::filesystem::path gCompilePath = "";
CShaderCompileCache gCompileCache = CShaderCompileCache();
std::filesystem::path gCompileCacheFile = "shaderCompileCache.txt";
std::string gTempOutputFile = "CompileResult.temp.txt";

void __init()
{
    _ASSERTE(!gInited);
    // find compiler path from environment variable
    std::vector<std::string> KeySet = { "VK_SDK_PATH", "VULKAN_SDK" };
    for (const auto& Key : KeySet)
    {
        auto Val = Environment::getEnvironmentVariable(Key);
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

bool __executeCompileWithOutput(const std::string& vCommand, std::string& voOutput)
{
    int ExitCode = std::system((vCommand + " > " + gTempOutputFile).c_str());
    voOutput = Common::readFileAsString(gTempOutputFile);
    return ExitCode == 0;
}

std::filesystem::path ShaderCompiler::compile(const std::filesystem::path& vPath)
{
    if (!gInited) __init();
    
    std::filesystem::path ActualPath;
    if (!Environment::findFile(vPath, true, ActualPath))
        throw std::runtime_error("File not found");

    auto OutputPath = __generateCompiledShaderPath(vPath);
    std::string Cmd = gCompilePath.string() + " -V \"" + vPath.string() + "\" -o \"" + OutputPath.string() + "\"";
    std::string Output;
    bool Success = __executeCompileWithOutput(Cmd, Output);

    // check if error
    if (!Success)
    {
        Log::log("Compile failed on file " + vPath.string());

        auto ShaderErrorMsg = ShaderErrorParser::parseAndFormat(Output);
        Log::log(ShaderErrorMsg);
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
