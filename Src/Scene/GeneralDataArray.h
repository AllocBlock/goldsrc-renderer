#pragma once
#include "DataArray.h"

#include <vector>

template <typename T>
class CGeneralDataArray : public IDataArray<T>
{
protected:
    virtual size_t _sizeV() override
    {
        return m_Array.size();
    }

    virtual void _resizeV(size_t vSize, T vFill) override
    {
        m_Array.resize(vSize, vFill);
    }

    virtual T _getV(size_t vIndex) override
    {
        return m_Array[vIndex];
    }

    virtual void _setV(size_t vIndex, T vData) override
    {
        m_Array[vIndex] = vData;
    }
    
    virtual void _appendV(T vData, size_t vNum) override
    {
        while (vNum--)
        {
            m_Array.emplace_back(vData);
        }
    }

    virtual void _clearV() override
    {
        m_Array.clear();
    }

private:
    std::vector<T> m_Array;
};

