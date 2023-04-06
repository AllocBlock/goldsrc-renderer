#pragma once
#include <map>
#include <stdexcept>
#include <string>
#include "AppInfo.h"

class CRerecordState
{
public:
    _DEFINE_PTR(CRerecordState);

    CRerecordState(CAppInfo::Ptr vAppInfo): m_pAppInfo(vAppInfo) {}

    void addField(const std::string& vName)
    {
        if (m_FieldTimeMap.find(vName) != m_FieldTimeMap.end())
            throw std::runtime_error("Field already exists");
        m_FieldTimeMap[vName] = m_pAppInfo->getImageNum();
    }

    void requestRecord(const std::string& vFieldName)
    {
        m_FieldTimeMap[vFieldName] = m_pAppInfo->getImageNum();
    }
    
    void requestRecordForAll()
    {
        for (auto& Pair : m_FieldTimeMap)
        {
            Pair.second = m_pAppInfo->getImageNum();
        }
    }

    // return if need record and minus time by 1
    bool consume(const std::string& vFieldName)
    {
        if (m_FieldTimeMap.at(vFieldName) > 0)
        {
            m_FieldTimeMap[vFieldName]--;
            return true;
        }
        return false;
    }
private:
    CAppInfo::Ptr m_pAppInfo = nullptr;
    std::map<std::string, size_t> m_FieldTimeMap;
};
