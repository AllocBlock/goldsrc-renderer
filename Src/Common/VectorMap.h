#pragma once
#include <vector>

/**
 * \brief A vector with map ability
 * save data as vector: iteration order is the same as insertion order
 * get data like map: key-value style, unique map
 */
template <typename Key_t, typename Value_t>
class CVectorMap
{
public:
    bool has(const Key_t& vKey) const
    {
        for (const auto& Pair : m_ItemSet)
            if (Pair.first == vKey)
                return true;
        return false;
    }

    size_t getIndex(const Key_t& vKey) const
    {
        size_t Index;
        if (__dumpIndexIfExist(vKey, Index))
            return Index;
        throw std::runtime_error("Item of key not found");
    }

    const Value_t& get(const Key_t& vKey) const
    {
        return m_ItemSet[getIndex(vKey)].second;
    }

    void set(const Key_t& vKey, const Value_t& vValue)
    {
        size_t Index;
        if (__dumpIndexIfExist(vKey, Index))
        {
            m_ItemSet[Index].second = vValue;
        }
        else
        {
            m_ItemSet.push_back({ vKey, vValue });
        }
    }

    void clear()
    {
        m_ItemSet.clear();
    }

    std::vector<Key_t> getKeys() const
    {
        std::vector<Key_t> KeySet;
        for (const auto& Pair : m_ItemSet)
            KeySet.emplace_back(Pair.first);
        return KeySet;
    }

private:
    bool __dumpIndexIfExist(const Key_t& vKey, size_t& voIndex) const
    {
        for (size_t i = 0; i < m_ItemSet.size(); ++i)
            if (m_ItemSet[i].first == vKey)
            {
                voIndex = i;
                return true;
            }
        return false;
    }

    std::vector<std::pair<Key_t, Value_t>> m_ItemSet;
};