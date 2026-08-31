#include "utils.h"
#include "picosha2.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <set>

bool is_old(const fs::path& p) {
    return fs::file_time_type::clock::now() - fs::last_write_time(p) > OLD_FILE_THRESHOLD;
}

double to_mb(uintmax_t bytes) {
    return bytes / BYTES_PER_MB;
}

std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

double get_directory_size(const fs::path& dir) {
    double total_size = 0;
    std::error_code ec;

    fs::recursive_directory_iterator it(dir, fs::directory_options::skip_permission_denied, ec);
    fs::recursive_directory_iterator end;

    while (it != end) {
        std::error_code file_ec;
        if (fs::is_regular_file(*it, file_ec) && !file_ec) {
            std::error_code size_ec;
            auto sz = fs::file_size(*it, size_ec);
            if (!size_ec) total_size += sz;
        }

        it.increment(ec);
        if (ec) {
            ec.clear();
        }
    }

    return total_size;
}

std::string hash_file(const fs::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return "";
    std::vector<unsigned char> hash(picosha2::k_digest_size);
    picosha2::hash256(f, hash.begin(), hash.end());
    return picosha2::bytes_to_hex_string(hash);
}


namespace {
    const std::set<std::string> kImageExtensions =
        {".jpg", ".jpeg", ".png", ".heic", ".bmp", ".gif", ".webp"};
}

bool is_image_file(const fs::path& p) {
    return kImageExtensions.count(to_lower(p.extension().string())) > 0;
}

bool move_to_trash(const fs::path& path) {
    std::string escaped;
    for (char ch : path.string()) {
        if (ch == '"') escaped += "\\\"";
        else escaped += ch;
    }
    std::string command =
        "osascript -e 'tell application \"Finder\" to delete POSIX file \""
        + escaped + "\"' >/dev/null 2>&1";
    return std::system(command.c_str()) == 0;
}