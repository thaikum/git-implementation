//
// Created by thaiku on 30/11/24.
//

#include "ChangesManager.h"
#include "data.h"
#include "IndexFileParser.h"
#include "../JitUtility/jit_utility.h"

#include <iostream>
#include <fstream>
#include <regex>
#include <ranges>
#include <set>

namespace fs = std::filesystem;

namespace manager {
    ChangesManager::ChangesManager(const std::string &root_directory) : DirManager(root_directory),
                                                                        root_directory(root_directory),
                                                                        jit_root(root_directory + "/.jit") {
    }

    void ChangesManager::populate_index_file() {
        std::map<std::string, FileInfo> current_files = get_files_map(files);
        IndexFileParser parser(current_files, jit_root + "/index");
        parser.create_index_file(current_files);
    }

    std::map<std::string, FileInfo> ChangesManager::get_files_map(const std::set<std::string> &files_to_add) {
        std::map<std::string, FileInfo> current_files;

        for (auto &file_name: files_to_add) {
            FileInfo file_info;
            file_info.filename = file_name;
            file_info.checksum = generateSHA1(root_directory + "/" + file_name);
            file_info.is_dirty = false;
            file_info.is_new = false;
            file_info.last_modified = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    fs::last_write_time(root_directory + "/" + file_name) - fs::file_time_type::clock::now() +
                    std::chrono::system_clock::now()
            );

            current_files.insert(std::pair(file_info.filename, file_info));
        }
        return current_files;
    }

    std::set<std::string> ChangesManager::transform_file_names() {
        auto temp_files = get_files();

        auto transformed_filenames = std::views::transform(temp_files, [](const std::string &file_name) {
            return std::regex_replace(file_name, std::regex("[\\./]+/"), "");
        });

        files = std::set(transformed_filenames.begin(), transformed_filenames.end());
        return files;
    }

    std::map<std::string, std::set<FileInfo>> ChangesManager::repo_status() {
        transform_file_names();
        auto file_map = get_files_map(files);
        IndexFileParser parser(file_map, jit_root + "/index");
        IndexFileContent previous_content = parser.read_index_file();
        std::set<FileInfo> new_files;
        std::set<FileInfo> modified_files;
        std::set<FileInfo> deleted_files;
        std::set<FileInfo> staged_files;

        for (auto const &file_info_pair: file_map) {
            auto file_name = file_info_pair.first;
            auto file_info = file_info_pair.second;

            if (previous_content.files_map.contains(file_name)) {
                auto old_file_info = previous_content.files_map.at(file_name);
                if (old_file_info.checksum != file_info.checksum) {
                    modified_files.insert(file_info);
                } else {
                    if (old_file_info.is_dirty) {
                        staged_files.insert(old_file_info);
                    }
                }

                previous_content.files_map.erase(file_name);
            } else {
                new_files.insert(file_info);
            }
        }

        std::map<std::string, std::set<FileInfo>> result;
        result.insert(std::pair("new_files", new_files));
        result.insert(std::pair("modified_files", modified_files));
        result.insert(std::pair("staged_files", staged_files));

        for (auto const &file_info_pair: previous_content.files_map) {
            deleted_files.insert(file_info_pair.second);
        }

        result.insert(std::pair("deleted_files", deleted_files));
        return result;
    }

    void ChangesManager::print_jit_status() {
        auto status = repo_status();
        auto new_files = status.at("new_files");
        auto modified_files = status.at("modified_files");
        auto deleted_files = status.at("deleted_files");
        auto staged_files = status.at("staged_files");

        std::ifstream head_file(jit_root + "/HEAD");

        if (head_file) {
            std::string head;
            getline(head_file, head);

            if (head.starts_with("refs")) {
                std::cout << "On branch " << GREEN << std::regex_replace(head, std::regex(".+/"), "") << RESET
                          << std::endl;
            } else {
                std::cout << "HEAD detached at " << CYAN << head.substr(0, 7) << RESET << std::endl;
            }
        } else {
            throw std::runtime_error("Cannot open the head file");
        }

        if (new_files.empty() && modified_files.empty() && deleted_files.empty() && staged_files.empty()) {
            std::cout << GREEN << "nothing to commit, working tree clean" << RESET << std::endl;
            return;
        }

        if (!staged_files.empty()) {
            std::cout << "\nChanges to be committed:" << std::endl;
            for (const auto &file_info: staged_files) {
                std::string operation = file_info.is_dirty ? "modified" : file_info.is_new ? "new file" : "deleted";
                std::string operation_color = file_info.is_dirty ? YELLOW : file_info.is_new ? GREEN : RED;
                std::cout << operation_color << "\t" << operation << ": " << RESET << file_info.filename << std::endl;
            }
        }

        if (!modified_files.empty() || !deleted_files.empty()) {
            std::cout << "\nChanges not staged for commit:" << std::endl;
            for (const auto &file_info: modified_files) {
                std::cout << YELLOW << "\tmodified: " << RESET << file_info.filename << std::endl;
            }

            for (const auto &file_info: deleted_files) {
                std::cout << RED << "\tdeleted: " << RESET << file_info.filename << std::endl;
            }
        }

        if (!new_files.empty()) {
            std::cout << "\nUntracked files:" << std::endl;
            for (const auto &file_info: new_files) {
                std::cout << RED << "\t" << file_info.filename << RESET << std::endl;
            }
        }
    }


    std::vector<std::string> ChangesManager::split_string(const std::string &str, const std::string &delimiter) {
        std::regex re(delimiter);
        std::sregex_token_iterator first{str.begin(), str.end(), re, -1}, last;
        return {first, last};
    }

    void ChangesManager::jit_add(const std::set<std::string> &file_names) {
        std::set<std::string> files_to_add;
        transform_file_names();

        for (auto &file_name: file_names) {
            fs::path path = root_directory + "/" + file_name;

            if (fs::is_directory(path)) {
                for (auto &directory: fs::recursive_directory_iterator(path)) {
                    auto cur_file_name = std::regex_replace(directory.path().string(), std::regex("[\\./]+/"), "");

                    if (!fs::is_directory(directory) && files.contains(cur_file_name))
                        files_to_add.insert(cur_file_name);
                }
            }
        }


        std::map<std::string, FileInfo> added_file_info = get_files_map(files_to_add);
        IndexFileParser parser(jit_root + "/index");
        parser.create_index_file(added_file_info);

        std::set<FileInfo> staged_files;
        for (const auto &file_pair: added_file_info) {
            staged_files.insert(file_pair.second);
        }
        update_file_objects(staged_files);
    }


    void ChangesManager::update_file_objects(const std::set<FileInfo> &file_infos) {
        for (const auto &file_info: file_infos) {
            save_as_binary(jit_root + "/objects", file_info.checksum, root_directory + "/" + file_info.filename);
        }
    }

} // manager