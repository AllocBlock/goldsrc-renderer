﻿#include "SceneInfo.h"

#include <iostream>

ptr<CIOImage> createSingleValueImage(size_t vWidth, size_t vHeight, uint8_t vValue)
{
    std::vector<uint8_t> TestData1(vWidth * vHeight * 4);
    for (uint8_t& Byte : TestData1)
        Byte = vValue;
    CIOImage::Ptr pImage = make<CIOImage>();
    pImage->setSize(vWidth, vHeight);
    pImage->setChannelNum(4);
    pImage->setData(TestData1.data());

    return pImage;
}

int main()
{
    CLightmap Lightmap;

    // test image 1, size 4x4, full of 1
    ptr<CIOImage> TestImage1 = createSingleValueImage(4, 4, 1);

    // test image 2, size 2x2, full of 2
    ptr<CIOImage> TestImage2 = createSingleValueImage(2, 2, 2);

    // test image 3, size 4x4, full of 3
    ptr<CIOImage> TestImage3 = createSingleValueImage(4, 4, 3);

    // add to lightmap
    Lightmap.appendLightmap(TestImage1);
    Lightmap.appendLightmap(TestImage2);
    Lightmap.appendLightmap(TestImage3);

    // print result
    ptr<CIOImage> pCombinedImage = Lightmap.getCombinedLightmap();
    const uint8_t* pIndices = reinterpret_cast<const uint8_t*>(pCombinedImage->getData());
    size_t ImageWidth = pCombinedImage->getWidth();
    std::cout << "lightmap result:" << std::endl;
    for (size_t i = 0; i < pCombinedImage->getHeight(); ++i)
    {
        for (size_t k = 0; k < pCombinedImage->getWidth(); ++k)
        {
            std::cout << static_cast<int>(pIndices[i * ImageWidth * 4 + k * 4]) << " ";
            std::cout << static_cast<int>(pIndices[i * ImageWidth * 4 + k * 4 + 1]) << " ";
            std::cout << static_cast<int>(pIndices[i * ImageWidth * 4 + k * 4 + 2]) << " ";
            std::cout << static_cast<int>(pIndices[i * ImageWidth * 4 + k * 4 + 3]) << ", ";
        }
        std::cout << std::endl;
    }
}