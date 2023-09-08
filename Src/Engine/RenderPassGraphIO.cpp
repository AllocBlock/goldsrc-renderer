#include "RenderPassGraphIO.h"
#include "Common.h"

#include <regex>
#include <sstream>

const std::regex gReGraphPart(R"(\[(.*?)\]([^\[]*))");
const std::regex gReGraphNode(R"X(\s*"([^"]*)"\s*"([^"]*)"\s*(-?\d*\.?\d*)\s*(-?\d*\.?\d*))X"); // FIXME: rough and brute float matching, fix it later
const std::regex gReGraphLink(R"X(\s*"([^"]*)"\s*\"([^"]*)"\s*"([^"]*)"\s*"([^"]*)"\s*)X");
const std::regex gReGraphEntry(R"X(\s*"([^"]*)"\s*"([^"]*)"\s*)X");

ptr<SRenderPassGraph> RenderPassGraphIO::load(const std::filesystem::path& vFilePath)
{
    ptr<SRenderPassGraph> pGraph = make<SRenderPassGraph>();

    std::string Data = Common::readFileAsString(vFilePath);

    std::sregex_iterator pPartIter(Data.begin(), Data.end(), gReGraphPart);
    std::sregex_iterator pEnd;
    
    std::map<std::string, size_t> PassNameIdMap;
    size_t CurNodeId = 0;
    size_t CurLinkId = 0;

    std::smatch MatchResult;
    while (pPartIter != pEnd)
    {
        const std::string& PartName = (*pPartIter)[1];
        const std::string& Content = (*pPartIter)[2];
        if (PartName == "Nodes")
        {
            const std::vector<std::string>& LineSet = Common::split(Content, "\n");
            for (std::string Line : LineSet)
            {
                Line = Common::trim(Line);
                if (Line.empty()) continue;
                if (std::regex_match(Line, MatchResult, gReGraphNode))
                {
                    const std::string& PassType = MatchResult[1];
                    const std::string& PassName = MatchResult[2];
                    float PosX = std::stof(MatchResult[3]);
                    float PosY = std::stof(MatchResult[4]);

                    if (PassNameIdMap.find(PassName) != PassNameIdMap.end())
                    {
                        throw std::runtime_error("Found duplicate pass name: " + PassName);
                    }
                    pGraph->NodeMap[CurNodeId] = SRenderPassGraphNode(PassType, glm::vec2(PosX, PosY));
                    PassNameIdMap[PassName] = CurNodeId;
                    CurNodeId++;
                }
                else
                {
                    throw std::runtime_error("Parse line in node part failed, the file is wrong?" + Line);
                }
            }
        }
        else if (PartName == "Links")
        {

            const std::vector<std::string>& LineSet = Common::split(Content, "\n");
            for (std::string Line : LineSet)
            {
                Line = Common::trim(Line);
                if (Line.empty()) continue;
                if (std::regex_match(Line, MatchResult, gReGraphLink))
                {
                    const std::string& SrcPassName = MatchResult[1];
                    const std::string& SrcPortName = MatchResult[2];
                    const std::string& DestPassName = MatchResult[3];
                    const std::string& DestPortName = MatchResult[4];
                    if (PassNameIdMap.find(SrcPassName) == PassNameIdMap.end())
                        throw std::runtime_error("Pass in link is not found: " + SrcPassName);
                    size_t SrcPassId = PassNameIdMap.at(SrcPassName);
                    if (PassNameIdMap.find(DestPassName) == PassNameIdMap.end())
                        throw std::runtime_error("Pass in link is not found: " + DestPassName);
                    size_t DestPassId = PassNameIdMap.at(DestPassName);

                    pGraph->LinkMap[CurLinkId] = { {SrcPassId, SrcPortName}, {DestPassId, DestPortName} };
                    CurLinkId++;
                }
                else
                {
                    throw std::runtime_error("Parse line in link part failed, the file is wrong?" + Line);
                }
            }
        }
        else if (PartName == "Output")
        {
            if (pGraph->OutputPort.has_value())
                throw std::runtime_error("Output port already set, only single Output part is allow in file");
            if (std::regex_match(Content, MatchResult, gReGraphEntry))
            {
                const std::string& PassName = MatchResult[1];
                const std::string& PortName = MatchResult[2];

                if (PassNameIdMap.find(PassName) == PassNameIdMap.end())
                    throw std::runtime_error("Pass in link is not found: " + PassName);
                size_t PassId = PassNameIdMap.at(PassName);
                pGraph->OutputPort = { PassId, PortName };
            }
            else
            {
                throw std::runtime_error("Output part can not be parsed, the file is wrong?" + Content);
            }
        }
        else
        {
            throw std::runtime_error("Unknown part found in graph file: " + PartName);
        }
        ++pPartIter;
    }

    return pGraph;
}

void RenderPassGraphIO::save(cptr<SRenderPassGraph> vGraph, const std::filesystem::path& vFilePath)
{
    std::map<size_t, std::string> PassIdNameMap;

    std::ostringstream Data;
    // Nodes
    if (!vGraph->NodeMap.empty())
    {
        Data << "[Nodes]\n";
        size_t CurId = 1;
        for (const auto& Pair : vGraph->NodeMap)
        {
            size_t NodeId = Pair.first;
            const SRenderPassGraphNode& Node = Pair.second;

            std::string PassName = std::to_string(CurId) + "_" + Node.Name;
            CurId++;

            PassIdNameMap[NodeId] = PassName;
            Data << "\"" << Node.Name << "\" \"" << PassName << "\" " << Node.Pos.x << " " << Node.Pos.y << "\n";
        }
    }
    // Links
    if (!vGraph->LinkMap.empty())
    {
        Data << "[Links]\n";
        for (const auto& Pair : vGraph->LinkMap)
        {
            const SRenderPassGraphLink& Link = Pair.second;

            std::string SrcPassName = PassIdNameMap.at(Link.Source.NodeId);
            std::string DestPassName = PassIdNameMap.at(Link.Destination.NodeId);

            Data << "\"" << SrcPassName << "\" \"" << Link.Source.Name << "\" \"" <<  DestPassName << "\" \"" << Link.Destination.Name << "\"\n";
        }
    }
    // Output
    if (vGraph->OutputPort.has_value())
    {
        Data << "[Output]\n";
        std::string PassName = PassIdNameMap.at(vGraph->OutputPort->NodeId);
        Data << "\"" + PassName + "\" \"" + vGraph->OutputPort->Name + "\"\n";
    }

    Common::writeStringToFile(Data.str(), vFilePath);
}