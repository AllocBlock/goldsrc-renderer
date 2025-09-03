#pragma once
#include "Buffer.h"

namespace vk
{
    class CVertexBuffer : public CBuffer
    {
    public:
        
        struct SSegmentInfo
        {
            uint32_t First;
            uint32_t Num;
        };
        
        void destroy()
        {
            m_VertexNum = 0;
            m_DataInfoSet.clear();
            CBuffer::destroy();
        }

        uint32_t getVertexNum() const
        {
            return m_VertexNum;
        }

        size_t getSegmentNum() const
        {
            return m_DataInfoSet.size();
        }

        const SSegmentInfo& getSegmentInfo(size_t vIndex) const
        {
            _ASSERTE(vIndex < m_DataInfoSet.size());
            return m_DataInfoSet[vIndex];
        }

    protected:
        uint32_t m_VertexNum = 0;
        std::vector<SSegmentInfo> m_DataInfoSet;

    private:
        using CBuffer::create;
        using CBuffer::stageFill;
        using CBuffer::fill;
        using CBuffer::copyFrom;
    };

    template <typename PointData_t>
    class CVertexBufferTyped : public CVertexBuffer
    {
    public:
        
        bool create(cptr<CDevice> vDevice, const std::vector<PointData_t>& vData)
        {
            return create(vDevice, std::vector<std::vector<PointData_t>>{ vData });
        }

        bool create(cptr<CDevice> vDevice, const std::vector<std::vector<PointData_t>>& vDataSet)
        {
            destroy();

            m_VertexNum = 0;
            m_DataInfoSet.clear();
            for (const auto& Data : vDataSet)
            {
                uint32_t CurVertexNum = static_cast<uint32_t>(Data.size());
                m_DataInfoSet.push_back({ m_VertexNum, CurVertexNum });
                m_VertexNum += CurVertexNum;
            }

            if (m_VertexNum == 0) return false;

            VkDeviceSize BufferSize = sizeof(PointData_t) * m_VertexNum;
            uint8_t* pData = new uint8_t[BufferSize];
            size_t Offset = 0;
            for (const auto& Data : vDataSet)
            {
                size_t SubBufferSize = sizeof(PointData_t) * Data.size();
                memcpy(reinterpret_cast<char*>(pData) + Offset, Data.data(), SubBufferSize);
                Offset += SubBufferSize;
            }
            
            CBuffer::create(vDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            CBuffer::stageFill(pData, BufferSize);
            delete[] pData;

            return true;
        }
    };
}
