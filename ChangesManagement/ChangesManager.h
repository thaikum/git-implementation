//
// Created by thaiku on 30/11/24.
//

#ifndef JIT_CHANGESMANAGER_H
#define JIT_CHANGESMANAGER_H

#include <string>
#include <vector>
#include <set>
#include <filesystem>
#include "../DirectoryManagent/DirManager.h"
#include "IndexFileParser.h"

namespace manager {

    class ChangesManager : public DirManager {
    public:
        explicit ChangesManager(const std::string &root_directory);

        void jit_add(const std::set<std::string> &file_names);

        static std::vector<std::string> split_string(const std::string &s, const std::string &delimiter);

        [[nodiscard]] JitStatus repo_status();

        void print_jit_status();

        std::set<std::string> transform_file_names();

    private:
        std::string jit_root;
        const std::string root_directory;
        std::set<std::string> files;

        void populate_index_file();

        std::map<std::string, FileInfo> get_files_map(const std::set<std::string> &files_to_add);

        void update_file_objects(const std::set<FileInfo> &file_infos);
    };

} // manager

#endif //JIT_CHANGESMANAGER_H
