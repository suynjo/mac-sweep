#pragma once
#include <filesystem>
#include <chrono>
#include <string>

namespace fs = std::filesystem;

constexpr auto OLD_FILE_THRESHOLD = std::chrono::days(7);
constexpr double BYTES_PER_MB = 1024.0 * 1024.0;

bool is_old(const fs::path& p);
double to_mb(uintmax_t bytes);
std::string to_lower(std::string s);
double get_directory_size(const fs::path& dir);
std::string hash_file(const fs::path& path);
bool is_image_file(const fs::path& p);
bool move_to_trash(const fs::path& path);