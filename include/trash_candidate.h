#pragma once
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

struct trash_candidate {
    fs::path path;
    double size_mb;
    std::string category;
    std::string reason;
};