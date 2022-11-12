#include "ShaderCompiler.h"
#include "Log.h"
#include "Common.h"
#include "Environment.h"
#include "ShaderCompileCache.h"
#include "ShaderErrorParser.h"

#include <set>

bool gInited = false;
std::filesystem::path gCompilePath = "";
CShaderCompileCache gCompileCache = CShaderCompileCache();
std::filesystem::path gCompileCacheFile = Environment::getTempFilePath("shaderCompileCache.txt");
std::filesystem::path gTempOutputFile = Environment::getTempFilePath("CompileResult.temp.txt");
std::set<std::filesystem::path> gHeaderDirSet;

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

    ShaderCompiler::addHeaderDir("./shaders");
    ShaderCompiler::addHeaderDir("../RenderPass/shaders");
    gInited = true;
}

std::filesystem::path __generateCompiledShaderPath(const std::filesystem::path& vPath)
{
    auto Ext = vPath.extension().string().substr(1);
    auto Postfix = char(std::toupper(Ext[0])) + Ext.substr(1);
    auto NewPath = "compiledShaders" / vPath.filename();
    NewPath.replace_extension("");
    NewPath += Postfix + ".spv";
    return Environment::getTempFilePath(NewPath);
}

bool __executeCompileWithOutput(const std::string& vCommand, std::string& voOutput)
{
    int ExitCode = std::system((vCommand + " > \"" + gTempOutputFile.string() + "\"").c_str());
    voOutput = Common::readFileAsString(gTempOutputFile);
    return ExitCode == 0;
}

void ShaderCompiler::addHeaderDir(const std::filesystem::path& vPath)
{
    gHeaderDirSet.insert(Environment::normalizePath(vPath));
}

std::filesystem::path ShaderCompiler::compile(const std::filesystem::path& vPath)
{
    if (!gInited) __init();
    
    std::filesystem::path ActualPath;
    if (!Environment::findFile(vPath, true, ActualPath))
        throw std::runtime_error("File not found");

    auto OutputPath = __generateCompiledShaderPath(vPath);
    std::string Cmd = gCompilePath.string() + " "; // exe 
    
    for (const auto& Dir : gHeaderDirSet)
        Cmd += "-I\"" + Dir.string() + "\" "; // additional include directory

    Cmd += "-V \"" + vPath.string() + "\" "; // input
    Cmd += "-o \"" + OutputPath.string() + "\" "; // output

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
