//
// Created by thaiku on 05/12/24.
//
#include <regex>
#include <future>
#include <iostream>
#include <utility>

#include "JitActions.h"
#include "data.h"
#include "IndexFileParser.h"

namespace manager {

    /**
     * Print the differences between files, showing additions and deletions in a diff-like format.
     *
     * @param diff A map where the key is the file name, and the value is a vector of strings representing the differences.
     *             Lines starting with '+' are additions, while lines starting with '-' are deletions.
     */
    void jit_print_diff(const std::map<std::string, std::vector<std::string>> &diff) {
        for (const auto &[file_name, changes] : diff) {
            std::cout << CYAN << "diff --jit a/" << file_name << " b/" << file_name << RESET << std::endl;

            for (const auto &line : changes) {
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

    /**
     * Get the changed files between two sets of file maps, comparing file content based on checksum.
     * Returns a map where the key is the file name and the value is a pair of original and updated file content.
     *
     * @param map1 A map containing the file information for the first set of files.
     * @param map2 A map containing the file information for the second set of files.
     * @return A map of filenames with pairs of their content before and after the changes.
     */
    std::unordered_map<std::string, std::pair<std::vector<std::string>, std::vector<std::string>>>
    JitActions::get_changed_files_data(const std::unordered_map<std::string, FileInfo> &map1,
                                       std::unordered_map<std::string, FileInfo> map2) {
        std::unordered_map<std::string, std::pair<std::vector<std::string>, std::vector<std::string>>> result;

        for (const auto &[file_name, file_info] : map1) {
            if (!(map2.contains(file_name) && map2[file_name].checksum == file_info.checksum)) {
                // Retrieve file content for both versions
                auto v1 = read_binary_as_text(get_root_directory() + "/objects/" + generate_file_path(file_info.checksum).string());
                auto v2 = map2.contains(file_name)
                          ? read_binary_as_text(get_root_directory() + "/objects/" + generate_file_path(map2[file_name].checksum).string())
                          : std::vector<std::string>{};

                result[file_name] = {v1, v2};
            }
            map2.erase(file_name); // Erase processed files
        }

        // Remaining files in map2 are new additions
        for (const auto &[file_name, file_info] : map2) {
            auto v2 = read_binary_as_text(get_root_directory() + "/objects/" + generate_file_path(file_info.checksum).string());
            result[file_name] = {{}, v2};
        }

        return result;
    }

    /**
     * Print differences between two branches, extracting the branch names from a given string format.
     *
     * @param branch_name A string representing the branch comparison (e.g., "branch1..branch2").
     *                    If the format is incorrect, it attempts to use the current HEAD and the given branch.
     */
    void JitActions::jit_diff(const std::string &branch_name) {
        std::regex diff_branch_extractor(R"(([A-Za-z0-9\._\-]+)\.\.([A-Za-z0-9\._\-]+))");
        std::smatch match;

        if (std::regex_search(branch_name, match, diff_branch_extractor)) {
            jit_diff(match[1], match[2]);
        } else {
            std::string head = get_head();
            jit_diff(std::regex_replace(head, std::regex(".+/"), ""), branch_name);
        }
    }

    /**
     * Print differences between two specific branches, by comparing their file contents.
     *
     * @param branch1 The first branch to compare.
     * @param branch2 The second branch to compare.
     */
    void JitActions::jit_diff(const std::string &branch1, const std::string &branch2) {
        // Create temporary files for branch indices
        auto branch1_index_temp = create_temp_file(get_branch_head(branch1));
        auto branch2_index_temp = create_temp_file(get_branch_head(branch2));

        // Read file maps from indices
        auto branch1_content = IndexFileParser(branch1_index_temp).read_index_file();
        auto branch2_content = IndexFileParser(branch2_index_temp).read_index_file();

        // Get changed files data
        auto changed_files_data = get_changed_files_data(branch1_content.files_map, branch2_content.files_map);

        // Compute diffs for changed files
        std::map<std::string, std::vector<std::string>> diff;
        for (const auto &[file_name, file_data] : changed_files_data) {
            diff[file_name] = compute_diff(file_data.first, file_data.second);
        }

        jit_print_diff(diff);
        fs::remove_all(get_root_directory() + "/temp/");
    }

    /**
     * Print differences between the current working directory and the repository state.
     * It checks the modified, deleted, and staged files, and shows the differences.
     */
    void JitActions::jit_diff() {
        const auto status = repo_status();

        if (status.modified_files.empty() && status.deleted_files.empty() && status.staged_files.empty()) {
            return; // No differences to display
        }

        // Launch asynchronous tasks to fetch original file content
        auto modified_files_future = std::async(std::launch::async, [this]() { return get_changed_files_content(); });


        // Prepare current file content
        std::map<std::string, std::vector<std::string>> current_content;
        for (const auto &file_set : {status.modified_files, status.staged_files}) {
            for (const auto &file : file_set) {
                current_content[file.filename] = read_file_to_vector(get_root_directory() + "/" + file.filename);
            }
        }

        // Add deleted files with empty content
        for (const auto &deleted_file : status.deleted_files) {
            current_content[deleted_file.filename] = {};
        }

        // Fetch original file content
        auto modified_files_content = modified_files_future.get();


        // Merge original content into a single map
        const std::map<std::string, std::vector<std::string>>& original_files = modified_files_content;


        // Calculate diffs
        std::map<std::string, std::vector<std::string>> diff_content_per_file;
        for (const auto &[filename, current_lines] : current_content) {
            const auto &original_lines = original_files.contains(filename) ? original_files.at(filename) : std::vector<std::string>{};
            diff_content_per_file[filename] = compute_diff(original_lines, current_lines);
        }

        jit_print_diff(diff_content_per_file);
    }

    /**
     * Retrieve the content of all files from the repository as a map of filenames to their content.
     * This method collects the file content for all files tracked in the repository at the current HEAD.
     *
     * @return A map where the key is the filename, and the value is a vector of strings representing the file's content.
     */
    std::map<std::string, std::vector<std::string>> JitActions::get_changed_files_content() {
        std::string head = get_head();
        fs::create_directory(get_root_directory() + "/temp");

        if (head.starts_with("refs")) {
            head = get_branch_head(std::regex_replace(head, std::regex(".+/"), ""));
        }

        IndexFileContent content = IndexFileParser(create_temp_file(head)).read_index_file();
        std::map<std::string, std::vector<std::string>> files_content;

        for (const auto &[_, file_info] : content.files_map) {
            std::string filename = get_root_directory() + "/objects/" + generate_file_path(file_info.checksum).string();
            files_content[file_info.filename] = read_binary_as_text(filename);
        }

        return files_content;
    }
}
