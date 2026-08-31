#pragma once
#include <vector>
#include "trash_candidate.h"

std::vector<trash_candidate> scan_files();
std::vector<trash_candidate> scan_user_caches();
std::vector<trash_candidate> scan_developer_caches();
std::vector<trash_candidate> scan_duplicates();
std::vector<trash_candidate> scan_installers();
std::vector<trash_candidate> collect_candidates();