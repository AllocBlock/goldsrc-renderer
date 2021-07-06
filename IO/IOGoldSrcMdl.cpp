#include "IOGoldSrcMdl.h"

#include <vector>
#include <array>

// 存储mdl文件内的纹理
// 从纹理块起始，若有n张纹理，则先是n个SMdlTexture数据
// 紧跟着是图片数据，其起始索引通过SMdlTexture内的DataOffset得到，先存储了Width*Height个8位索引，紧跟256色24位调色板
struct SMdlTexture
{
    char Name[64];
    int32_t FlagBitField;
    int32_t Width;
    int32_t Height;
    int32_t	DataOffset;
    std::vector<uint8_t> IndexSet;
    std::array<IOCommon::SGoldSrcColor, 256> Palette;

    static size_t getHeaderSize()
    {
        return 64 * sizeof(char) + 4 * sizeof(int32_t);
    }

    void read(std::ifstream& voFile)
    {
        voFile.read(reinterpret_cast<char*>(this), getHeaderSize());
        voFile.seekg(DataOffset);
        IndexSet.resize(Width * Height);
        voFile.read(reinterpret_cast<char*>(IndexSet.data()), Width * Height);
        voFile.read(reinterpret_cast<char*>(Palette.data()), 256 * 3 * sizeof(float));
    }
};

struct SMdlMesh
{
    int numtris;
    int triindex;
    int skinref;
    int numnorms;		// per mesh normals
    int normindex;		// normal vec3_t
};

struct SMdlModel
{
    char				name[64];

    int					type;

    float				boundingradius;

    int					nummesh;
    int					meshindex;

    int					numverts;		// number of unique vertices
    int					vertinfoindex;	// vertex bone info
    int					vertindex;		// vertex vec3_t
    int					numnorms;		// number of unique surface normals
    int					norminfoindex;	// normal bone info
    int					normindex;		// normal vec3_t

    int					numgroups;		// deformation groups
    int					groupindex;
};

struct SMdlBodyPart
{
    char Name[64];
    int32_t ModelNum;
    int32_t Base;
    int32_t ModelDataOffset;
};

struct SMdlSkin
{
    std::vector<int16_t> RefToTextureIndex;

    void read(std::ifstream& voFile, int vSkinNum)
    {
        RefToTextureIndex.resize(vSkinNum);
        voFile.read(reinterpret_cast<char*>(RefToTextureIndex._Get_data()), vSkinNum * sizeof(int16_t));
    }
};

bool CIOGoldSrcMdl::_readV(std::filesystem::path vFilePath)
{
    std::ifstream File(vFilePath, std::ios::in | std::ios::binary);
    if (!File.is_open())
        throw std::runtime_error(u8"文件读取失败");

    File.read(reinterpret_cast<char*>(&m_Header), sizeof(SMdlHeader));
    _ASSERTE(m_Header.Magic[0] == 'I' &&
        m_Header.Magic[1] == 'D' &&
        m_Header.Magic[2] == 'S' &&
        (m_Header.Magic[3] == 'T' || m_Header.Magic[3] == 'Q'));

    // 读取纹理
    std::vector<SMdlTexture> TextureSet(m_Header.TextureNum);
    const size_t TextureHeaderSize = SMdlTexture::getHeaderSize();
    for (int i = 0; i < m_Header.TextureNum; ++i)
    {
        File.seekg(m_Header.TextureDataIndex + i * TextureHeaderSize);
        TextureSet[i].read(File);
    }
    
    // 读取部件

}