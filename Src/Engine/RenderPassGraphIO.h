#pragma once
#include "Pointer.h"
#include "RenderPassGraph.h"

#include <filesystem>

/**
 * render pass graph file io
 * render pass graph file is a ascii text file, with .graph as extension
 * a graph file contains 3 things:
 * - render pass nodes, where each node refer to a renderpass
 * - pass links, which connect input and output of passes
 * - an entry, which refer to a port where the base render target (e.g., swapchain image) enters
 * a example of graph file looks like this:
 * [Nodes]
 * "GoldSrc"   "1_GoldSrc" 0 0
 * "Outline"   "2_Outline" 200 200
 * "Visualize" "3_Visualize" 400 0
 * "Gui"       "4_Gui" 600 0
 *
 * [Links]
 * "1_GoldSrc"   "Main"  "2_Outline"   "Main"
 * "2_Outline"   "Main"  "3_Visualize" "Main"
 * "3_Visualize" "Main"  "4_Gui"       "Main" 
 * "1_GoldSrc"   "Depth" "3_Visualize" "Depth"
 *
 * [Output]
 * "4_Gui"   "Main"
 *
 * each node start with the pass type (identifier) and a unique pass name, then two float indicate the position of node
 * each link contain src pass name, output port name, dest pass name, input port name
 * entry contain entry pass name and the entry port name
 */
namespace RenderPassGraphIO
{
    sptr<SRenderPassGraph> load(const std::filesystem::path& vFilePath);
    void save(cptr<SRenderPassGraph> vGraph, const std::filesystem::path& vFilePath);
};