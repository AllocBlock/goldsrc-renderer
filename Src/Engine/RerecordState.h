#pragma once
#include <map>
#include <stdexcept>
#include <string>

class CRerecordState
{
public:
    _DEFINE_PTR(CRerecordState);

    CRerecordState(size_t vImageNum): m_ImageNum(vImageNum) {}

    void addField(const std::string& vName)
    {
        if (m_FieldTimeMap.find(vName) != m_FieldTimeMap.end())
            throw std::runtime_error("Field already exists");
        m_FieldTimeMap[vName] = m_ImageNum;
    }

    void requestRecord(const std::string& vFieldName)
    {
        m_FieldTimeMap[vFieldName] = m_ImageNum;
    }
    
    void requestRecordForAll()
    {
        for (auto& Pair : m_FieldTimeMap)
        {
            Pair.second = m_ImageNum;
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
    size_t m_ImageNum = 0;
    std::map<std::string, size_t> m_FieldTimeMap;
};
