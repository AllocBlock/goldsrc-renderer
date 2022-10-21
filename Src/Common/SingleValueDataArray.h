#pragma once
#include "DataArray.h"

template <typename T>
class CSingleValueDataArray : public IDataArray<T>
{
public:
    CSingleValueDataArray() = default;
    CSingleValueDataArray(T vValue, size_t vSize) : m_Value(vValue), m_Size(vSize)
    {
    }

protected:
    virtual size_t _sizeV() override
    {
        return m_Size;
    }

    virtual void _resizeV(size_t vSize, T vFill) override
    {
        m_Size = vSize;
    }

    virtual T _getV(size_t vIndex) override
    {
        return m_Value;
    }

    virtual void _setV(size_t vIndex, T vData) override
    {
        m_Value = vData;
    }

    virtual void _appendV(T vData, size_t vNum) override
    {
        m_Value = vData;
    }

    virtual void _clearV() override
    {
        m_Value = T();
    }

    virtual IDataArray<T>::Ptr _copyV() override
    {
        auto pNewArray = make<CSingleValueDataArray<T>>();
        pNewArray->m_Size = m_Size;
        pNewArray->m_Value = m_Value;
        return pNewArray;
    }

private:
    size_t m_Size = 0;
    T m_Value = T();
};

