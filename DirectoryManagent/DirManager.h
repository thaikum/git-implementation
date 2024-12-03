//
// Created by thaiku on 30/11/24.
//

#ifndef JIT_DIRMANAGER_H
#define JIT_DIRMANAGER_H

#include <set>
#include <string>
#include <filesystem>
#include <map>

namespace manager {

    class DirManager {
    public:
        explicit DirManager(const std::string &);
        explicit DirManager(const char *);

        [[nodiscard]] std::set<std::string> get_files();

        void initialize_jit();

        [[nodiscard]] std::string get_jit_directory();

    private:
        std::string root_directory;
        std::string jit_directory;
        std::set<std::string> files;
        std::string ignore_file_names_regex_construction;
        std::string ignore_directory_regex_construction;

    protected:
        void get_nested_files_in_a_directory(const std::filesystem::path &dir);

        void update_repository(const std::set<std::string> &files_to_delete,
                               const std::map<std::string, std::string> &files_to_modify);
    };

} // manager

#endif //JIT_DIRMANAGER_H
