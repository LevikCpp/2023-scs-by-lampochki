#ifndef VCS_H
#define VCS_H

#include <string>
#include <unordered_map>
#include <vector>

bool vcs_init();
bool vcs_add(const std::vector<std::string>& args);
bool vcs_commit(const std::string& message);
bool vcs_log();

std::unordered_map<std::string, std::string> create_snapshot();

#endif