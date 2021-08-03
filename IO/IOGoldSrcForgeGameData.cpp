#include "IOGoldSrcForgeGameData.h"
#include "Log.h"

#include <regex>

using namespace Common;

#define _CHECK_ERROR(X) __checkResult(X, __LINE__)

size_t CIOGoldSrcForgeGameData::getEntityNum() const
{
    return m_EntitySet.size();
}

const SFGDEntity& CIOGoldSrcForgeGameData::getEntity(size_t vIndex) const
{
    _ASSERTE(vIndex < m_EntitySet.size());
    return m_EntitySet[vIndex];
}

std::string CIOGoldSrcForgeGameData::__removeQuote(std::string vText)
{
    std::regex ReWindowNewline(R"(\r\n)");
    vText = std::regex_replace(vText, ReWindowNewline, "\n");
    std::regex ReQuote(R"(//.*?\n)");
    vText = std::regex_replace(vText, ReQuote, "");
    return vText;
}

std::vector<std::string> CIOGoldSrcForgeGameData::__splitFileToEntityStrings(std::string vText)
{
    std::vector<std::string> Result;

    std::regex ReEntity(R"(@[^@]*)");

    std::regex_iterator<std::string::const_iterator> ReBegin(vText.cbegin(), vText.cend(), ReEntity);
    for (auto pIter = ReBegin; pIter != std::sregex_iterator(); ++pIter)
    {
        Result.emplace_back(pIter->str());
    }

    return Result;
}

std::vector<std::string> __splitAttributeStr(std::string vText)
{
    // 通过逗号分割，不过属性值内可能出现括号，所以要考虑深度
    std::vector<std::string> Result;
    size_t StartIndex = 0;
    int Depth = 0;
    for (size_t i = 0; i < vText.length(); ++i)
    {
        if (vText[i] == '(')
            Depth++;
        else if (vText[i] == ')')
        {
            Depth--;
            if (Depth == 0)
            {
                Result.emplace_back(vText.substr(StartIndex, i - StartIndex + 1));
                StartIndex = i + 1;
            }
        }
    }
    return Result;
}

SFGDAttribute CIOGoldSrcForgeGameData::__parseAttributeFromString(std::string vText)
{
    SFGDAttribute Result;

    std::smatch MatchResult;
    std::regex ReKeyValue(R"ALLOCBLOCK((.*)\((.*)\))ALLOCBLOCK");

    _CHECK_ERROR(std::regex_search(vText, MatchResult, ReKeyValue));
    Result.Name = CIOBase::trimString(MatchResult[1].str());
    Result.Value = CIOBase::trimString(MatchResult[2].str());
    
    return Result;
}

std::vector<SFGDAttribute> CIOGoldSrcForgeGameData::__parseAttributeSetFromString(std::string vText)
{
    std::vector<SFGDAttribute> Result;

    auto AttributeStringSet = __splitAttributeStr(vText);
    for (const auto& AttributeString : AttributeStringSet)
    {
        auto Attribute = __parseAttributeFromString(AttributeString);
        Result.emplace_back(Attribute);
    }

    return Result;
}

std::pair<std::string, std::string> CIOGoldSrcForgeGameData::__parseNameAndDescriptionFromString(std::string vText)
{
    std::string Name, Description;

    std::smatch MatchResult;
    std::regex ReName(R"ALLOCBLOCK(=\s*([^:]*))ALLOCBLOCK");
    std::regex ReDescription(R"ALLOCBLOCK(:\s*(".*"\s(\+\s*".*"\s)*))ALLOCBLOCK");

    if (std::regex_search(vText, MatchResult, ReName))
        Name = CIOBase::trimString(MatchResult[1].str());

    if(std::regex_search(vText, MatchResult, ReName))
        Description = CIOBase::trimString(MatchResult[1].str());

    return std::make_pair(Name, Description);
}

std::vector<SFGDKeyValueInfo> CIOGoldSrcForgeGameData::__parseKeyValueInfoSetFromString(std::string vText)
{
    std::vector<SFGDKeyValueInfo> Result;
    // TODO: 暂时用不到
    return Result;
}

SFGDEntity CIOGoldSrcForgeGameData::__parseEntityFromString(std::string vText)
{
    SFGDEntity Entity;

    std::smatch MatchResult;
    std::regex ReEntity(R"ALLOCBLOCK(@(\S*)\s+([^=]*)(=([^[]*)(:([^[]*))?)?\[([\s\S]*)\])ALLOCBLOCK");
    _CHECK_ERROR(std::regex_search(vText, MatchResult, ReEntity));

    Entity.Type = MatchResult[1].str();
    Entity.AttributeSet = __parseAttributeSetFromString(MatchResult[2].str());
    std::tie(Entity.Name, Entity.Description) = __parseNameAndDescriptionFromString(MatchResult[3].str());
    Entity.KeyValueInfoSet = __parseKeyValueInfoSetFromString(MatchResult[7].str());

    return Entity;
}

bool CIOGoldSrcForgeGameData::_readV(std::filesystem::path vFilePath)
{
    std::ifstream File;
    File.open(vFilePath, std::ios::in);
    if (!File.is_open())
    {
        Log::log(u8"打开文件 [" + vFilePath.u8string() + u8"] 失败，无权限或文件不存在");
        return false;
    }

    std::string Text((std::istreambuf_iterator<char>(File)), std::istreambuf_iterator<char>());
    Text = __removeQuote(Text);

    auto EntityStringSet = __splitFileToEntityStrings(Text);
    for (const auto& EntityString : EntityStringSet)
    {
        auto Entity = __parseEntityFromString(EntityString);
        m_EntitySet.emplace_back(Entity);
    }
}

void CIOGoldSrcForgeGameData::__checkResult(bool vResult, int vLine)
{
    if (!vResult)
    {
        throw std::runtime_error(u8"解析FGD文件失败，位于行" + std::to_string(vLine));
    }
}