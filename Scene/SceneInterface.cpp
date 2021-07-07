#include "SceneInterface.h"
#include "SceneReaderObj.h"
#include "SceneReaderRmf.h"
#include "SceneReaderMap.h"
#include "SceneReaderBsp.h"
#include "SceneReaderMdl.h"

#include <string>
#include <functional>
#include <algorithm>

using ReadFunc_t = std::function<std::shared_ptr<SScene>(std::filesystem::path, std::function<void(std::string)>)>;
std::map<std::string, ReadFunc_t> g_RegistedList;

template <typename T>
struct SRegister
{
    SRegister() = delete;
    SRegister(std::string vType)
    {
        std::transform(vType.begin(), vType.end(), vType.begin(), ::tolower);
        g_RegistedList[vType] = [](std::filesystem::path vFilePath, std::function<void(std::string)> vProgressReportFunc) -> std::shared_ptr<SScene>
        {
            T Reader;
            return Reader.read(vFilePath, vProgressReportFunc);
        };
    }
};

#define REGISTER_FILE(CLASS, TYPE) SRegister<CLASS> g_Register##CLASS##(TYPE);

REGISTER_FILE(CSceneReaderObj, "obj");
REGISTER_FILE(CSceneReaderRmf, "rmf");
REGISTER_FILE(CSceneReaderMap, "map");
REGISTER_FILE(CSceneReaderBsp, "bsp");
REGISTER_FILE(CSceneReaderMdl, "mdl");

std::shared_ptr<SScene> SceneReader::read(std::string vType, std::filesystem::path vFilePath, std::function<void(std::string)> vProgressReportFunc)
{
    std::transform(vType.begin(), vType.end(), vType.begin(), ::tolower);
    if (g_RegistedList.count(vType) == 0)
        throw std::runtime_error(u8"未知的文件类型");
    else
        return g_RegistedList[vType](vFilePath, vProgressReportFunc);
}