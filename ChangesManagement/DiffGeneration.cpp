//
// Created by thaiku on 05/12/24.
//
#include <regex>
#include <future>
#include <ranges>
#include <iostream>

#include "JitActions.h"
#include "data.h"

namespace manager {
    void jit_print_diff(const std::map<std::string, std::vector<std::string>> &diff) {
        for (const auto &[file_name, changes]: diff) {
            std::cout << CYAN << "diff --git a/" << file_name << " b/" << file_name << RESET << std::endl;

            for (const auto &line: changes) {
                if (line.empty()) continue;

                if (line[0] == '+') {
                    std::cout << GREEN << line << RESET << std::endl;
                } else if (line[0] == '-') {
                    std::cout << RED << line << RESET << std::endl;
                } else {
                    std::cout << line << std::endl;
                }
            }

            std::cout << std::endl;
        }
    }

    void JitActions::jit_diff() {
        const auto status = repo_status();


        if (!status.modified_files.empty()) {
            auto modified_file_names = std::views::transform(status.modified_files,
                                                             [](const auto &val) { return val.filename; });

            std::future original_files_future = std::async(std::launch::async,
                                                           [this, files = status.modified_files, modified_file_names] {
                                                               return get_changed_files_content(
                                                                       std::set(modified_file_names.begin(),
                                                                                modified_file_names.end()));
                                                           });

            const auto modified_files_content = status.modified_files;
            std::map<std::string, std::vector<std::string>> current_content;

            for (auto &modified_file: status.modified_files) {
                current_content.insert(
                        {modified_file.filename,
                         read_file_to_vector(get_root_directory() + "/" + modified_file.filename)});
            }
            auto original_files = original_files_future.get();

            std::map<std::string, std::vector<std::string>> diff_content_per_file;

            for (const auto &current_content_pair: current_content) {
                diff_content_per_file.insert({current_content_pair.first,
                                              diff_files(original_files.at(current_content_pair.first),
                                                         current_content_pair.second)});
            }

            jit_print_diff(diff_content_per_file);
        }
    }

    std::map<std::string, std::vector<std::string>>
    JitActions::get_changed_files_content(const std::set<std::string> &files_to_read) {
        std::string head = get_head();
        fs::create_directory(jit_root + "/temp");

        if (head.starts_with("refs")) {
            head = get_branch_head(std::regex_replace(head, std::regex(".+/"), ""));
        }

        IndexFileContent content = IndexFileParser(create_temp_file(head)).read_index_file();
        std::map<std::string, std::vector<std::string>> files_content;

        for (auto &[_, file_info]: content.files_map) {
            std::string filename = jit_root + "/objects/" + generate_file_path(file_info.checksum).string();
            files_content.insert({file_info.filename, read_binary_as_text(filename)});
        }

        return files_content;
    }
}