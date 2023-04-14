#include "SceneInterface.h"
#include "SceneReaderObj.h"
#include "SceneReaderRmf.h"
#include "SceneReaderMap.h"
#include "SceneReaderBsp.h"
#include "SceneReaderMdl.h"

#include <string>
#include <functional>
#include <algorithm>

using ReadFunc_t = std::function<void(std::filesystem::path, ptr<SSceneInfo>)>;
std::map<std::string, ReadFunc_t> g_RegistedList;

template <typename T>
struct SRegister
{
    SRegister() = delete;
    SRegister(std::string vType)
    {
        std::transform(vType.begin(), vType.end(), vType.begin(), ::tolower);
        g_RegistedList[vType] = [](std::filesystem::path vFilePath, ptr<SSceneInfo> voSceneInfo)
        {
            T Reader;
            Reader.read(vFilePath, voSceneInfo);
        };
    }
};

#define REGISTER_FILE(CLASS, TYPE) SRegister<CLASS> g_Register##CLASS##(TYPE);

REGISTER_FILE(CSceneReaderObj, "obj");
REGISTER_FILE(CSceneReaderRmf, "rmf");
REGISTER_FILE(CSceneReaderMap, "map");
REGISTER_FILE(CSceneReaderBsp, "bsp");
REGISTER_FILE(CSceneReaderMdl, "mdl");

void SceneInterface::read(std::string vType, std::filesystem::path vFilePath, ptr<SSceneInfo> voSceneInfo)
{
    _ASSERTE(voSceneInfo);
    std::transform(vType.begin(), vType.end(), vType.begin(), ::tolower);
    if (g_RegistedList.count(vType) == 0)
        throw std::runtime_error(u8"未知的文件类型" + vType);
    else
        g_RegistedList[vType](vFilePath, voSceneInfo);
}