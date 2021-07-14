#pragma once
#include "IOGoldsrcWad.h"

#include <vector>
#include <filesystem>
#include <functional>

std::vector<CIOGoldsrcWad> readWads(const std::vector<std::filesystem::path>& vWadPaths);