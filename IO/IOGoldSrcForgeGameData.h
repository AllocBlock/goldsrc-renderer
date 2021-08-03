#pragma once
#include "IOBase.h"
#include "IOCommon.h"

struct SFGDAttribute
{
    std::string Name;
    std::string Value;
};

struct SFGDKeyValueInfoItem
{
    std::string Value;
    std::string DisplayName;
    std::string Default;
};

struct SFGDKeyValueInfo
{
    std::string Name;
    std::string Type;
    std::string DisplayName;
    std::string Default;
    std::vector<SFGDKeyValueInfoItem> AttributeSet;
};

struct SFGDEntity
{
    std::string Type;
    std::string Name;
    std::string Description;
    std::vector<SFGDAttribute> AttributeSet;
    std::vector<SFGDKeyValueInfo> KeyValueInfoSet;
};

// FGDÎÄ¼þ
// https://developer.valvesoftware.com/wiki/FGD
class CIOGoldSrcForgeGameData : public CIOBase
{
public:
    CIOGoldSrcForgeGameData() : CIOBase() {}
    CIOGoldSrcForgeGameData(std::filesystem::path vFilePath) : CIOBase(vFilePath) {}

    size_t getEntityNum() const;
    const SFGDEntity& getEntity(size_t vIndex) const;

protected:
    virtual bool _readV(std::filesystem::path vFilePath) override;

private:
    void __checkResult(bool vResult, int vLine);
    std::string __removeQuote(std::string vText);
    std::vector<std::string> __splitFileToEntityStrings(std::string vText);
    SFGDEntity __parseEntityFromString(std::string vText);
    std::vector<SFGDAttribute> __parseAttributeSetFromString(std::string vText);
    std::pair<std::string, std::string> __parseNameAndDescriptionFromString(std::string vText);
    std::vector<SFGDKeyValueInfo> __parseKeyValueInfoSetFromString(std::string vText);
    SFGDAttribute __parseAttributeFromString(std::string vText);

    std::vector<SFGDEntity> m_EntitySet;
};

