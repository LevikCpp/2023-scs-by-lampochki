#include "vcs.h"

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

int get_next_snapshot_id() {
    int snapshot_id = 0;
    std::string snapshot_dir = ".archive/snapshot_" + std::to_string(snapshot_id);
    while (fs::exists(snapshot_dir)) {
        snapshot_id++;
        snapshot_dir = ".archive/snapshot_" + std::to_string(snapshot_id);
    }
    return snapshot_id;
}

void create_snapshot_directory(int snapshot_id) {
    std::string snapshot_dir = ".archive/snapshot_" + std::to_string(snapshot_id);
    fs::create_directory(snapshot_dir);
}

void copy_from_to(const fs::path &src_path, const fs::path &dst_path) {
    std::ifstream src(src_path, std::ios::binary);
    std::ofstream dst(dst_path, std::ios::binary);
    dst << src.rdbuf();
}

std::string compute_file_hash(const fs::path &path) {
    std::ifstream src(path, std::ios::binary);
    src.clear();
    src.seekg(0, std::ios::beg);
    std::istreambuf_iterator<char> src_begin(src);
    std::istreambuf_iterator<char> src_end;
    std::string data(src_begin, src_end);
    std::hash<std::string> hash_fn;
    std::size_t hash_value = hash_fn(data);

    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(sizeof(std::size_t) * 2) << hash_value;
    return ss.str();
}

std::set<std::string> get_filenames_from_stage(fs::path archive_dir = ".archive") {
    std::ifstream stage_file_stream(archive_dir / ".stage");

    std::set<std::string> filenames;

    // empty string
    while (stage_file_stream) {
        std::string filename;
        stage_file_stream >> filename;
        filenames.insert(filename);
    }

    return filenames;
}

bool vcs_init() {
    fs::path archive_dir = ".archive";
    if (fs::exists(archive_dir) && fs::is_directory(archive_dir)) {
        return false;
    }

    fs::create_directory(archive_dir);
    std::cout << "Created directory " << archive_dir << std::endl;

    std::ofstream outfile((archive_dir / ".stage").string());

    int snapshot_id = get_next_snapshot_id();
    create_snapshot_directory(snapshot_id);
    auto snapshot_dir = fs::path(".archive/snapshot_" + std::to_string(snapshot_id));
    std::ofstream stage_file_stream(snapshot_dir / ".stage");

    return true;
}

bool vcs_add(const std::vector<std::string> &args) {
    fs::path archive_dir = ".archive";
    std::ofstream stage_file_stream(archive_dir / ".stage");

    for (auto &&filename : args) {
        if (fs::exists(filename)) {
            stage_file_stream << filename << '\n';
        } else {
            std::cout << '\'' << filename << "' does not exist and can't be added\n";
            return false;
        }
    }

    return true;
}

bool vcs_commit(const std::string &message) {
    create_snapshot();
    std::cout << "Committing changes with message: " << message << std::endl;

    return true;
}

std::unordered_map<std::string, std::string> create_snapshot() {
    int snapshot_id = get_next_snapshot_id();
    create_snapshot_directory(snapshot_id);
    auto snapshot_dir = fs::path(".archive/snapshot_" + std::to_string(snapshot_id));

    std::unordered_map<std::string, std::string> file_hashes;

    auto current_stage = get_filenames_from_stage();
    // copy file from current stage
    for (const auto &filename : current_stage) {
        fs::path curPath(filename);
        if (!fs::is_directory(curPath) && curPath.string().rfind(".\\.", 0) != 0) {
            copy_from_to(curPath, snapshot_dir / curPath.filename());
            auto key = curPath.string();
            file_hashes[key] = compute_file_hash(curPath);
        }
    }

    // copy files that are not in steger from parent
    auto prev_snap_dir = fs::path(".archive/snapshot_" + std::to_string(snapshot_id - 1));
    auto parent_stage = get_filenames_from_stage(prev_snap_dir);
    for (const auto &filename : parent_stage) {
        // check if file already copied from current stage
        if (current_stage.find(filename) == current_stage.end()) {
            // if file not in current stage, but in prev stage copy it from parent0
            fs::path curPath(filename);
            curPath = prev_snap_dir / curPath.filename();
            copy_from_to(curPath, snapshot_dir / curPath.filename());
            auto key = curPath.string();
            file_hashes[key] = compute_file_hash(curPath);
        }
    }

    // megre stages
    std::ofstream stage_file_stream(snapshot_dir / ".stage");
    current_stage.merge(parent_stage);
    for (auto &&filename : current_stage) {
        stage_file_stream << filename << '\n';
    }

    // log
    auto hashes_log_file_path = fs::path(snapshot_dir) / std::string("hashes.log");
    std::ofstream hashes_log_file(hashes_log_file_path);
    for (const auto &[file, hash] : file_hashes) {
        hashes_log_file << file << ' ' << hash << '\n';
    }
    hashes_log_file.close();

    return file_hashes;
}

bool vcs_log() {
    std::cout << "Log message" << std::endl;

    return true;
}
