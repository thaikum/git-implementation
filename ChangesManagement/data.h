//
// Created by thaiku on 01/12/24.
//
#ifndef DATA_H
#define DATA_H

#include <string>
#include <chrono>
#include <vector>
#include <map>
#include <set>

struct FileInfo {
    std::string filename;
    std::string checksum;
    std::chrono::system_clock::time_point addition_date;
    std::chrono::system_clock::time_point last_modified;
    bool is_dirty;
    bool is_new;
    bool is_deleted = false;

    bool operator<(const FileInfo& other) const {
        return filename < other.filename;
    }
};

struct IndexMetaData {
    size_t entries;
    std::chrono::system_clock::time_point last_modified;
    bool is_dirty;
};

struct IndexFileContent {
    IndexMetaData metaData;
    std::vector<FileInfo> files;
    std::map<std::string, FileInfo> files_map;
};

struct JitStatus{
    std::set<FileInfo> new_files;
    std::set<FileInfo> modified_files;
    std::set<FileInfo> deleted_files;
    std::set<FileInfo> staged_files;
};
#endif
