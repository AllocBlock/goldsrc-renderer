#pragma once
#include <map>
#include <stdexcept>
#include <string>

class CRerecordState
{
public:
    
    void addField(const std::string& vName)
    {
        if (m_Fields.find(vName) != m_Fields.end())
            throw std::runtime_error("Field already exists");
        m_Fields.insert(vName);
    }

    void requestRecord(const std::string& vFieldName)
    {
        m_DirtyFields.insert(vFieldName);
    }
    
    void requestRecordForAll()
    {
        for (const auto& field : m_Fields)
        {
            m_DirtyFields.insert(field);
        }
    }

    // return if need record and minus time by 1
    bool consume(const std::string& vFieldName)
    {
        if (m_DirtyFields.find(vFieldName) != m_DirtyFields.end())
        {
            m_DirtyFields.erase(vFieldName);
            return true;
        }
        return false;
    }
private:
    std::set<std::string> m_Fields;
    std::set<std::string> m_DirtyFields;
};
