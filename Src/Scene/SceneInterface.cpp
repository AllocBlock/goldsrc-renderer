#include "SceneInterface.h"
#include "SceneReaderObj.h"
#include "SceneReaderRmf.h"
#include "SceneReaderMap.h"
#include "SceneReaderBsp.h"
#include "SceneReaderMdl.h"

#include <string>
#include <functional>
#include <algorithm>

using ReadFunc_t = std::function<ptr<SSceneInfoGoldSrc>(std::filesystem::path)>;
std::map<std::string, ReadFunc_t> g_RegistedList;

template <typename T>
struct SRegister
{
    SRegister() = delete;
    SRegister(std::string vType)
    {
        std::transform(vType.begin(), vType.end(), vType.begin(), ::tolower);
        g_RegistedList[vType] = [](std::filesystem::path vFilePath) -> ptr<SSceneInfoGoldSrc>
        {
            T Reader;
            return Reader.read(vFilePath);
        };
    }
};

#define REGISTER_FILE(CLASS, TYPE) SRegister<CLASS> g_Register##CLASS##(TYPE);

REGISTER_FILE(CSceneReaderObj, "obj");
REGISTER_FILE(CSceneReaderRmf, "rmf");
REGISTER_FILE(CSceneReaderMap, "map");
REGISTER_FILE(CSceneReaderBsp, "bsp");
REGISTER_FILE(CSceneReaderMdl, "mdl");

ptr<SSceneInfoGoldSrc> SceneInterface::read(std::string vType, std::filesystem::path vFilePath)
{
    std::transform(vType.begin(), vType.end(), vType.begin(), ::tolower);
    if (g_RegistedList.count(vType) == 0)
        throw std::runtime_error(u8"δ֪���ļ�����");
    else
        return g_RegistedList[vType](vFilePath);
}