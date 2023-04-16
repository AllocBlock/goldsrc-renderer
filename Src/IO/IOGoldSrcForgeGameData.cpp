#include "IOGoldSrcForgeGameData.h"
#include "Log.h"

#include <regex>
#include <algorithm>

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

std::vector<std::string> __splitKeyValueStr(std::string vText)
{
    std::vector<std::string> Result;

    std::regex ReKeyValue(R"ALLOCBLOCK(\w*\(\w*\)(\s*:\s*(("[^"]*")|([^\[\]\n="]*)))*\s*(\s*=\s*\[[^\]]*\])?)ALLOCBLOCK");

    std::regex_iterator<std::string::const_iterator> ReBegin(vText.cbegin(), vText.cend(), ReKeyValue);
    for (auto pIter = ReBegin; pIter != std::sregex_iterator(); ++pIter)
    {
        Result.emplace_back(pIter->str());
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

std::vector<SFGDKeyValueInfoCandidate> CIOGoldSrcForgeGameData::__parseCandidateSetFromString(std::string vText)
{
    std::vector<SFGDKeyValueInfoCandidate> Result;

    std::smatch MatchResult;
    std::regex ReCandidateChoice(R"ALLOCBLOCK(.*:.*)ALLOCBLOCK"); // 匹配包含:符号的一行
    std::regex ReCandidateChoiceDetail(R"ALLOCBLOCK(([^:]*)\s*:\s*("[^"]*")(?:\s*:\s*([^:\s]*))?)ALLOCBLOCK");

    std::regex_iterator<std::string::const_iterator> ReBegin(vText.cbegin(), vText.cend(), ReCandidateChoice);
    for (auto pIter = ReBegin; pIter != std::sregex_iterator(); ++pIter)
    {
        std::string CandidateStr = pIter->str();
        _CHECK_ERROR(std::regex_search(CandidateStr, MatchResult, ReCandidateChoiceDetail));
        SFGDKeyValueInfoCandidate Candidate;
        Candidate.Value = __convertToUTF8(MatchResult[1].str());
        Candidate.DisplayName = __convertToUTF8(MatchResult[2].str());
        Candidate.Default = __convertToUTF8(MatchResult[3].str());
        Result.emplace_back(std::move(Candidate));
    }

    return Result;
}

SFGDKeyValueInfo CIOGoldSrcForgeGameData::__parseKeyValueFromString(std::string vText)
{
    SFGDKeyValueInfo Result;

    std::smatch MatchResult;
    std::regex ReNameAndType(R"ALLOCBLOCK(^\s*(\w*)\((\w*)\))ALLOCBLOCK");
    std::regex ReKeyValueNormal(R"ALLOCBLOCK(^\s*(\w*)\((\w*)\)\s*(?::\s*("[^"]*"))?\s*(?::\s*([^:]*))?\s*(?::\s*("[^"]*"))?\s*$)ALLOCBLOCK");
    std::regex ReKeyValueChoice(R"ALLOCBLOCK(^\s*(\w*)\((\w*)\)\s*(?::\s*("[^"]*"))?\s*(?::\s*([^:]*))?\s*(?::\s*("[^"]*"))?\s*=\s*\[([^\]]*)\]$)ALLOCBLOCK");


    _CHECK_ERROR(std::regex_search(vText, MatchResult, ReNameAndType));
    std::string Type = CIOBase::trimString(MatchResult[2].str());
    std::transform(Type.begin(), Type.end(), Type.begin(), [&](char vChar)->char { return std::tolower(vChar); });
    if (Type == "string" || Type == "integer" || Type == "target_source" || Type == "target_destination" ||
        Type == "color255" || Type == "color1" || Type == "sprite" || Type == "sound" || Type == "studio")
    {
        _CHECK_ERROR(std::regex_match(vText, MatchResult, ReKeyValueNormal));
    }
    else if (Type == "choices" || Type == "flags" || Type == "Flags")
    {
        _CHECK_ERROR(std::regex_match(vText, MatchResult, ReKeyValueChoice));
        Result.CandidateSet = __parseCandidateSetFromString(MatchResult[5].str());
    }
    else
    {
        throw std::runtime_error(u8"未知的FGD键值类型：" + Type);
    }
    Result.Name = __convertToUTF8(MatchResult[1].str());
    Result.Type = __convertToUTF8(MatchResult[2].str());
    Result.DisplayName = __convertToUTF8(MatchResult[3].str());
    Result.Default = __convertToUTF8(MatchResult[4].str());
    Result.Description = __convertToUTF8(MatchResult[5].str());
    
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

    auto KeyValueStringSet = __splitKeyValueStr(vText);
    for (const auto& KeyValueString : KeyValueStringSet)
    {
        auto KeyValue = __parseKeyValueFromString(KeyValueString);
        // TODO: 获取父类的属性一并加入
        Result.emplace_back(KeyValue);
    }
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
        Log::log("打开文件 [" + vFilePath.u8string() + "] 失败，无权限或文件不存在");
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
    return true;
}

void CIOGoldSrcForgeGameData::__checkResult(bool vResult, int vLine)
{
    if (!vResult)
    {
        throw std::runtime_error(u8"解析FGD文件失败，位于行" + std::to_string(vLine));
    }
}

std::string CIOGoldSrcForgeGameData::__convertToUTF8(std::string vText)
{
    std::filesystem::path vPath(vText);
    return vPath.u8string();
}