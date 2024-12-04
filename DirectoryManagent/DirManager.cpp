//
// Created by thaiku on 30/11/24.
//

#include "DirManager.h"
#include "../JitUtility/jit_utility.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <regex>

namespace fs = std::filesystem;

namespace manager {
    DirManager::DirManager(const char *root_directory) : root_directory(root_directory) {
        fs::path dir = root_directory;
    }

    DirManager::DirManager(const std::string &root_directory) : root_directory(root_directory) {
        fs::path dir = root_directory;
        get_nested_files_in_a_directory(dir);
    }

    std::set<std::string> DirManager::get_files() {
        get_nested_files_in_a_directory(root_directory);
        return files;
    }

    std::string DirManager::get_root_directory() {
        return root_directory;
    }

    void DirManager::initialize_jit() {
        jit_directory = root_directory + ".jit";

        if (fs::exists(jit_directory)) {
            throw std::runtime_error("Jit is already initialized for this directory");
        }

        fs::create_directory(jit_directory);

        const std::vector<std::string> subdirs = {
                "/branches",
                "/logs",
                "/logs/refs",
                "/logs/refs/heads",
                "/refs",
                "/refs/heads",
                "/objects"
        };

        for (const auto &subdir: subdirs) {
            fs::create_directory(jit_directory + subdir);
        }

        std::string master_file_path = jit_directory + "/refs/heads/master";
        std::ofstream master_file(master_file_path);

        if (master_file) {
            master_file << std::string(40, '0');
            master_file.close();
        } else {
            std::cerr << "Error creating the master file at: " << master_file_path << std::endl;
        }

        std::string log_master_file_path = jit_directory + "/logs/refs/heads/master";
        std::ofstream log_master_file(log_master_file_path);

        if (log_master_file) {
            log_master_file.close();
        } else {
            std::cerr << "Error creating the empty master file at: " << log_master_file_path << std::endl;
        }

        std::string index_file_path = jit_directory + "/index";
        std::ofstream index_file(index_file_path);

        if (index_file) {
            index_file.close();
        } else {
            std::cerr << "Error creating the empty index file at: " << index_file_path << std::endl;
        }

        std::string head_file_path = jit_directory + "/HEAD";
        std::ofstream head_file(head_file_path);

        if (head_file) {
            head_file << "refs/heads/master";
            head_file.close();
        } else {
            std::cerr << "Error creating the empty head file at: " << head_file_path << std::endl;
        }

        std::cout << "Initialized empty jit repository" << std::endl;
    }

    void DirManager::update_repository(const std::set<std::string> &files_to_delete,
                                       const std::map<std::string, std::string> &files_to_modify) {
        for (auto &file: files_to_delete) {
            fs::remove(root_directory + "/" + file);
        }

        for (auto &modified_file_pair: files_to_modify) {
            decompress_and_copy(modified_file_pair.first, root_directory + "/" + modified_file_pair.second + "");
        }
    }

    void DirManager::get_nested_files_in_a_directory(const std::filesystem::path &dir) {
        std::ifstream jit_ignore(root_directory + "/.jitignore");

        ignore_directory_regex_construction = ".jit";
        ignore_file_names_regex_construction = "";

        if (jit_ignore) {
            std::string line;
            bool has_started = false;
            files.clear();

            while (std::getline(jit_ignore, line)) {
                std::regex asterisk("\\*");
                std::string new_regex = std::regex_replace(line, asterisk, ".+");

                std::string prefix = has_started ? "|" : "";

                if (new_regex.back() == '/') {
                    ignore_directory_regex_construction += (prefix + new_regex);
                } else {
                    ignore_file_names_regex_construction += (prefix + new_regex);
                }
                has_started = true;
            }
        }

        if (fs::exists(dir) && fs::is_directory(dir)) {
            std::regex ignore_individual_files_regex(ignore_file_names_regex_construction);
            std::regex ignore_directories_regex(ignore_directory_regex_construction);

            for (const auto &entry: fs::recursive_directory_iterator(dir)) {
                if (!entry.is_directory()) {
                    std::smatch match, d_match;
                    std::string path_string = entry.path().string();
                    std::string file_name = entry.path().filename().string();

                    //todo fix this line to handle when there is no gitignore
                    if ((ignore_file_names_regex_construction.empty() && ignore_directory_regex_construction.empty()) ||
                        (!(!ignore_file_names_regex_construction.empty() &&
                           std::regex_search(file_name, match, ignore_individual_files_regex)) &&
                         !(!ignore_directory_regex_construction.empty() &&
                           std::regex_search(path_string, d_match, ignore_directories_regex)))) {
                        files.insert(entry.path().relative_path());
                    }
                }
            }
        }
    }
}