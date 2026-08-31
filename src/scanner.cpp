#include "scanner.h"
#include "utils.h"
#include <cstdlib>
#include <set>
#include <unordered_map>
#include <fstream>
#include "picosha2.h"

namespace { 
    const std::set<std::string> image_extensions = {".jpg", ".jpeg", ".png", ".heic"};
    const std::set<std::string> installer_extensions = {".dmg", ".pkg"};
}

// 1. old files
std::vector<trash_candidate> scan_files(){
    std::vector<trash_candidate> candidates;

    std::vector<fs::path> directories = {
        fs::path(std::getenv("HOME")) / "Desktop",
        fs::path(std::getenv("HOME")) / "Downloads",
        fs::path(std::getenv("HOME")) / "Documents"
    };

    for (const auto& directory:directories){
        if (!fs::exists(directory)) continue;

        for (const auto& file:fs::recursive_directory_iterator(directory, fs::directory_options::skip_permission_denied)){
            if (!fs::is_regular_file(file)) continue;

            // 1-1. old scrrenshots
            std::string file_name=to_lower(file.path().filename().string());
            if ((file_name.find("screenshot") != std::string::npos || file_name.find("스크린샷") != std::string::npos) 
            && is_old(file)){
                candidates.push_back(
                    trash_candidate {
                        .path=file,
                        .size_mb = to_mb(fs::file_size(file)),
                        .category="screenshot",
                        .reason="[screenshot] 7+ days passed"
                    } 
                );
            }

            // 1-2. old images
            else if (image_extensions.find(to_lower(file.path().extension().string()))!=image_extensions.end()
            && is_old(file)){
                candidates.push_back(
                    trash_candidate {
                        .path=file,
                        .size_mb = to_mb(fs::file_size(file)),
                        .category="image",
                        .reason="[image] 7+ days passed"
                    }  
                );
            }  
        }
    }
    return candidates;   
}

// 2. old caches
// 2-1. user caches
std::vector<trash_candidate> scan_user_caches(){
    std::vector<trash_candidate> candidates;

    fs::path cache_base = fs::path(std::getenv("HOME")) / "Library" / "Caches";

    if (fs::exists(cache_base)){
        for (const auto& cache_folder:fs::directory_iterator(cache_base, fs::directory_options::skip_permission_denied)){
            if (fs::is_directory(cache_folder)){
                std::string folder_name = to_lower((cache_folder.path().filename().string()));
                if (folder_name.find("apple") != std::string::npos) continue;

                if (is_old(cache_folder)){
                    candidates.push_back(
                        trash_candidate {
                            .path=cache_folder,
                            .size_mb = to_mb(get_directory_size(cache_folder)),
                            .category="user_cache",
                            .reason="[user_cache] 7+ days passed"
                        }  
                    );            
                }
            }
        }
    }
    return candidates;
}

// 2-2. developer caches
std::vector<trash_candidate> scan_developer_caches(){
    std::vector<trash_candidate> candidates;

    std::vector<fs::path> directories = {
        fs::path(std::getenv("HOME")) / ".npm" / "_cacache",
        fs::path(std::getenv("HOME")) / ".gradle" / "caches",
        fs::path(std::getenv("HOME")) / "Library" / "Developer" / "Xcode"
    };

    for (const auto& directory:directories){
        if (!fs::exists(directory)) continue;

        if (is_old(directory)){
            candidates.push_back(
                trash_candidate {
                    .path=directory,
                    .size_mb = to_mb(get_directory_size(directory)),
                    .category="developer_cache",
                    .reason="[developer_cache] 7+ days passed"
                }  
            );            
        }
    }
    return candidates;
}

// 3. duplicated_files

std::vector<trash_candidate> scan_duplicates(){
    std::vector<trash_candidate> candidates;

    std::unordered_map<uintmax_t, std::vector<fs::path>> files_by_size;
    fs::path directory = fs::path(std::getenv("HOME")) / "Downloads";

    if (!fs::exists(directory)) return candidates;

    for (const auto& file : fs::recursive_directory_iterator(directory, fs::directory_options::skip_permission_denied)){
        if (fs::is_regular_file(file)){
            uintmax_t size = fs::file_size(file);
            files_by_size[size].push_back(file.path());
        }
    }

    for (const auto& [size, file_list] : files_by_size){
        if (file_list.size() > 1){
            std::unordered_map<std::string, std::string> seen_hashes;

            for (const auto& file : file_list){
                std::string hash = hash_file(file);
                if (hash.empty()) continue;
                if (seen_hashes.find(hash) == seen_hashes.end()){
                    seen_hashes[hash] = file;
                } else {
                    candidates.push_back(
                        trash_candidate {
                            .path = file,
                            .size_mb = to_mb(fs::file_size(file)),
                            .category = "duplicate",
                            .reason = "[duplicate] duplicated files"
                        }
                    );
                }
            }
        }
    }
    return candidates;
}

// 4. old installers
std::vector<trash_candidate> scan_installers(){
    std::vector<trash_candidate> candidates;

    fs::path directory = fs::path(std::getenv("HOME")) / "Downloads";
    if (!fs::exists(directory)) return candidates;

    for (const auto& file :fs::recursive_directory_iterator(directory, fs::directory_options::skip_permission_denied)){

        if (!fs::is_regular_file(file)) continue;

        if (installer_extensions.find(to_lower(file.path().extension().string()))!=installer_extensions.end() 
        && is_old(file)){
            candidates.push_back(
                trash_candidate {
                    .path = file,
                    .size_mb = to_mb(fs::file_size(file)),
                    .category = "installer",
                    .reason = "[installer] 7+ days passed"
                }
            );
        }
    }
    return candidates;
}

std::vector<trash_candidate> collect_candidates() {
    std::vector<trash_candidate> all;

    auto append = [&all](std::vector<trash_candidate> v) {
        all.insert(all.end(),
                   std::make_move_iterator(v.begin()),
                   std::make_move_iterator(v.end()));
    };

    append(scan_files());
    append(scan_user_caches());
    append(scan_developer_caches());
    append(scan_duplicates());
    append(scan_installers());

    std::unordered_map<fs::path, trash_candidate> unique_candidates;
    for (auto& candidate : all) {
        auto it = unique_candidates.find(candidate.path);
        if (it == unique_candidates.end()) {
            unique_candidates.emplace(candidate.path, std::move(candidate));
        } else {
            it->second.reason += " / " + candidate.reason;
        }
    }

    std::vector<trash_candidate> result;
    result.reserve(unique_candidates.size());
    for (auto& [path, candidate] : unique_candidates) {
        result.push_back(std::move(candidate));
    }
    return result;
}