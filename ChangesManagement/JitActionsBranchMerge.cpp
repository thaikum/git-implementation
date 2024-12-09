//
// Created by thaiku on 04/12/24.
//

#include <fstream>
#include <regex>
#include <future>
#include <iostream>
#include <unordered_set>

#include "JitActions.h"
#include "../CommitManagement/CommitGraph.h"

namespace manager {

    /**
     * Retrieves the commit tree from a specified branch file.
     *
     * @param file_name The path to the branch file.
     * @return A map where the key is the base commit and the value is the new commit.
     * @throws std::runtime_error if the branch file does not exist.
     */
    std::map<std::string, std::string> get_commit_tree(const std::string &file_name) {
        std::regex commit_regex(
                R"((^[0-9a-f]{40})\s([0-9a-f]{40})\s(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})\s+(?:commit|merge):\s(.+))");
        std::smatch match;
        std::ifstream branch_file(file_name);
        std::map<std::string, std::string> branch_tree;

        if (branch_file) {
            std::string log_line;
            std::string last_commit;
            while (getline(branch_file, log_line)) {
                if (std::regex_search(log_line, match, commit_regex)) {
                    std::string base_commit = match[1].str();
                    std::string new_commit = match[2].str(); // Use the second SHA1 hash

                    branch_tree.insert(std::pair(base_commit, new_commit));
                    last_commit = new_commit;
                }
            }
            branch_tree.insert({last_commit, ""});
            branch_file.close();
            return branch_tree;
        } else {
            throw std::runtime_error(
                    "Branch: " + std::regex_replace(file_name, std::regex(".+/"), "") + " does not exist!");
        }
    }

    std::stack<std::string> JitActions::get_commit_stack(const std::string &file_name) {
        std::set<std::string> commits;
        std::stack<std::string> ordered_commits;

        std::regex commit_regex(
                R"((^[0-9a-f]{40})\s([0-9a-f]{40})\s(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})\s+(?:commit|merge):\s(.+))");
        std::smatch match;
        std::ifstream branch_file(file_name);
        std::map<std::string, std::string> branch_tree;

        if (branch_file) {
            std::string log_line;
            std::string last_commit;
            while (getline(branch_file, log_line)) {
                if (std::regex_search(log_line, match, commit_regex)) {
                    std::string base_commit = match[1].str();
                    std::string new_commit = match[2].str(); // Use the second SHA1 hash

                    if (!commits.contains(base_commit)) {
                        commits.insert(base_commit);
                        ordered_commits.push(base_commit);
                    }

                    if (!commits.contains(new_commit)) {
                        ordered_commits.push(new_commit);
                        commits.insert(new_commit);
                    }

                }
            }
            branch_file.close();
            return ordered_commits;
        } else {
            throw std::runtime_error(
                    "Branch: " + std::regex_replace(file_name, std::regex(".+/"), "") + " does not exist!");
        }
    }

    /**
     * Finds the intersection commit between two branches.
     *
     * @param branch1 The name of the first branch.
     * @param branch2 The name of the second branch.
     * @return The commit SHA1 of the common ancestor commit.
     * @throws std::runtime_error if the branches do not intersect.
     */
    std::string JitActions::get_intersection_commit(const std::string &branch1, const std::string &branch2) {
        auto base_branch_commit_tree = get_commit_tree(get_jit_root() + "/logs/refs/heads/" + branch1);
        auto feature_branch_commit_tree = get_commit_tree(get_jit_root() + "/logs/refs/heads/" + branch2);

        for (auto &[commit, _]: feature_branch_commit_tree) {
            if (base_branch_commit_tree.contains(commit)) {
                return commit;
            }
        }

        throw std::runtime_error("The branches do not intersect");
    }

    /**
     * Merges the specified feature branch into the current branch.
     *
     * @param feature_branch The name of the feature branch to be merged.
     * @throws std::runtime_error if the merge cannot be performed, such as when not on a branch.
     */
    void JitActions::merge(const std::string &feature_branch) {
        std::string head = get_head();
        std::string feature_branch_sum = feature_branch;

        if (head.starts_with("refs")) {
            std::string branch_name = std::regex_replace(head, std::regex(".+/"), "");

            std::string feature_branch_head = get_branch_head(feature_branch);
            std::string head_checksum = get_branch_head(branch_name);
            throw_error_if_repo_is_dirty();

            std::string commit_file = get_jit_root() + "/objects/" + generate_file_path(COMMIT_FILE_HASH).string();
            CommitGraph commit_graph(commit_file);

            if (commit_graph.get_commit(feature_branch) == nullptr) {
                feature_branch_sum = get_branch_head(feature_branch);
            }

            auto base_commit_ptr = commit_graph.get_intersection_commit(feature_branch_sum, head_checksum);

            if (base_commit_ptr == nullptr) {
                throw std::runtime_error("The branches are not related! Orphan merge out of scope");
            } else if (base_commit_ptr->checksum == feature_branch_sum) {
                throw std::runtime_error("No changes");
            }

            std::string base_commit = base_commit_ptr->checksum;

            fs::path base_index_file = fs::path(get_jit_root() + "/objects") / generate_file_path(base_commit);
            fs::path feature_branch_index_file =
                    fs::path(fs::path(get_jit_root() + "/objects")) / generate_file_path(feature_branch_head);


            IndexFileContent base_content = IndexFileParser::read_binary_index_file(get_jit_root() + "/objects/" +
                                                                                    generate_file_path(
                                                                                            base_commit).string());
            IndexFileContent feature_branch_content = IndexFileParser::read_binary_index_file(
                    get_jit_root() + "/objects/" +
                    generate_file_path(feature_branch_sum).string());

            IndexFileParser main_parser(get_jit_root() + "/index");
            IndexFileContent main_branch = main_parser.read_index_file();

            auto f_branch = feature_branch_content.files_map;
            auto b_branch = base_content.files_map;

            std::vector<FileInfo> merged_files;
            std::unordered_map<std::string, FileInfo> merged_files_map;
            std::unordered_set<std::string> files_with_conflicts;

            for (auto &main_pair: main_branch.files_map) {
                std::string absolute_path = get_root_directory() + "/" + main_pair.first;
                std::shared_ptr<bool> has_conflicts = std::make_unique<bool>(false);


                if (b_branch.contains(main_pair.first)) {
                    const auto &base = b_branch.at(main_pair.first);
                    bool file_in_f = f_branch.contains(main_pair.first);

                    if ((base.checksum == main_pair.second.checksum)) {

                        // File modified by the feature branch
                        if (file_in_f && f_branch.at(main_pair.first).checksum != main_pair.second.checksum) {
                            auto ff = f_branch.at(main_pair.first);
                            decompress_and_copy(
                                    (get_jit_root() + "/objects/" + generate_file_path(ff.checksum).string()),
                                    absolute_path);
                            merged_files.push_back(ff);
                            merged_files_map.insert({ff.filename, std::move(ff)});
                        } else {
                            merged_files.push_back(main_pair.second);
                            merged_files_map.insert(main_pair);
                        }

                        f_branch.erase(main_pair.first);
                    } else {
                        if (file_in_f && f_branch.at(main_pair.first).checksum != base.checksum) {
                            std::string temp_file_path = create_temp_file(main_pair.second.checksum);
                            std::string temp_base = create_temp_file(base.checksum);

                            std::future<std::vector<std::string>> branch_file_future = std::async(
                                    std::launch::async, read_file_to_vector, temp_file_path);
                            std::future<std::vector<std::string>> main_file_future = std::async(std::launch::async,
                                                                                                read_file_to_vector,
                                                                                                absolute_path);
                            std::future<std::vector<std::string>> base_file_future = std::async(std::launch::async,
                                                                                                read_file_to_vector,
                                                                                                temp_base);

                            const std::vector<std::string> merged_vector = three_way_merge(branch_file_future.get(),
                                                                                           base_file_future.get(),
                                                                                           main_file_future.get(),
                                                                                           has_conflicts);
                            write_vector_to_file(absolute_path, merged_vector);
                            auto sha = generateSHA1(absolute_path);

                            if (*has_conflicts) {
                                main_pair.second.is_dirty = true;
                            }
                            main_pair.second.checksum = sha;
                            merged_files.push_back(main_pair.second);
                            merged_files_map.insert(main_pair);
                            f_branch.erase(main_pair.first);
                        } else {
                            merged_files.push_back(main_pair.second);
                            merged_files_map.insert(main_pair);
                        }
                    }
                } else if (f_branch.contains(main_pair.first)) {
                    // File present in both branches but absent in base
                    std::string temp_branch_file = create_temp_file(f_branch.at(main_pair.first).checksum);
                    const auto branch_file_vector = read_file_to_vector(temp_branch_file);
                    const auto main_file_vector = read_file_to_vector(absolute_path);
                    const std::vector<std::string> base;

                    const auto merged_vector = three_way_merge(base, branch_file_vector, main_file_vector,
                                                               has_conflicts);

                    if (*has_conflicts) {
                        main_pair.second.is_dirty = true;
                    }
                    auto sha = generateSHA1(absolute_path);
                    main_pair.second.checksum = sha;

                    write_vector_to_file(absolute_path, merged_vector);
                    f_branch.erase(main_pair.first);
                } else {
                    merged_files.push_back(main_pair.second);
                    merged_files_map.insert(main_pair);
                }

                if (*has_conflicts) {
                    files_with_conflicts.insert(main_pair.first);
                }
            }

            for (auto &f_pair: f_branch) {
                std::string absolute_path = get_root_directory() + "/" + f_pair.first;
                decompress_and_copy(
                        (get_jit_root() + "/objects/" + generate_file_path(f_pair.second.checksum).string()),
                        absolute_path);
                merged_files.push_back(f_pair.second);
                merged_files_map.insert({f_pair.second.filename, std::move(f_pair.second)});
            }

            if (files_with_conflicts.empty()) {
                main_branch.files_map = merged_files_map;
                main_parser.write_index_file(main_branch);

                Commit commit;
                commit.checksum = generateSHA1(get_jit_root() + "/index");
                commit.message = "Merge " + feature_branch + " into " + branch_name;
                commit.timestamp = std::chrono::system_clock::now();
                commit.branch_name = branch_name;

                commit_graph.add_commit(commit, {feature_branch_sum, head_checksum});
                commit_graph.save_commits(commit_file);
                std::cout << "Merged " + feature_branch + " into " + branch_name << std::endl;
                save_as_binary(get_jit_root() + "/objects", commit.checksum, get_jit_root() + "/index");


                update_branch_head_file(branch_name, commit.checksum);
                update_head_file(head);

                jit_log(get_jit_root() + "/logs/refs/heads/" + branch_name, head_checksum, commit.checksum,
                        "merge: fast forward");
            } else {
                std::cout << "Automatic merge failed. The following files have conflicts. Resolve them and then commit"
                          << std::endl;
                for (const auto &file: files_with_conflicts) {
                    std::cout << RED << file << RESET << std::endl;
                }
            }
        } else {
            throw std::runtime_error("Cannot perform merge while outside a branch");
        }
    }

    /**
     * Creates a temporary file for a given checksum.
     *
     * @param checksum The checksum of the file.
     * @return The path to the newly created temporary file.
     */
    std::string JitActions::create_temp_file(const std::string &checksum) {
        if (!fs::exists(get_jit_root() + "/temp/")) {
            fs::create_directory(get_jit_root() + "/temp/");
        }

        std::string new_file_name = get_jit_root() + "/temp/" + checksum;
        decompress_and_copy((get_jit_root() + "/objects/" + generate_file_path(checksum).string()), new_file_name);
        return new_file_name;
    }

    /**
     * Reads the content of a file into a vector of strings, each representing a line.
     *
     * @param filename The path to the file.
     * @return A vector of strings representing the lines of the file.
     * @throws std::runtime_error if the file cannot be opened.
     */
    std::vector<std::string> JitActions::read_file_to_vector(const std::string &filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + filename);
        }

        std::vector<std::string> lines;
        std::string line;
        while (getline(file, line)) {
            lines.push_back(line);
        }

        file.close();
        return lines;
    }

/**
 * @brief Writes the contents of a vector of strings to a file, each string on a new line.
 *
 * This function takes a vector of strings and writes each string to a file. The strings are written to
 * the file with a newline separating them. If the file already exists, it will be overwritten.
 *
 * @param filename The name of the file to write the data to.
 * @param lines A vector of strings to be written to the file, each string will appear on its own line.
 */
    void JitActions::write_vector_to_file(const std::string &filename, const std::vector<std::string> &lines) {
        std::ofstream file(filename);

        // Iterate over each line in the vector and write it to the file.
        for (auto &line: lines) {
            file << line << std::endl;
        }
        file.close(); // Close the file after writing.
    }

/**
 * @brief Merges three versions of a file (base, branch_1, and branch_2) using a three-way merge algorithm.
 *
 * This function performs a three-way merge between three vectors of strings representing the base version,
 * branch_1, and branch_2 of a file. The algorithm compares each line from the three versions, and:
 * - If the lines from both branches match the base, the merged version is the base line.
 * - If only one branch differs, the merged version uses the changes from that branch.
 * - If both branches differ, it creates a merge conflict marker in the merged version.
 *
 * @param base A vector of strings representing the base version of the file.
 * @param branch_1 A vector of strings representing the first branch version of the file.
 * @param branch_2 A vector of strings representing the second branch version of the file.
 * @return A vector of strings representing the merged version of the file. If conflicts are found,
 *         conflict markers (<<<<<<<, =======, >>>>>>>) are inserted into the merged file.
 */
    std::vector<std::string> JitActions::three_way_merge(
            const std::vector<std::string> &base,
            const std::vector<std::string> &branch_1,
            const std::vector<std::string> &branch_2, const std::shared_ptr<bool> &has_conflict) {

        std::vector<std::string> merged;
        size_t i = 0, j = 0, k = 0;

        // Loop through the base, branch_1, and branch_2 vectors to compare lines.
        while (i < base.size() || j < branch_1.size() || k < branch_2.size()) {
            std::string base_line = (i < base.size()) ? base[i] : "";
            std::string branch_1_line = (j < branch_1.size()) ? branch_1[j] : "";
            std::string branch_2_line = (k < branch_2.size()) ? branch_2[k] : "";

            // If both branches have the same line as the base, use that line.
            if (branch_1_line == branch_2_line) {
                std::cout << "Attempting merging" << std::endl;
                merged.push_back(branch_1_line);
                if (i < base.size() && branch_1_line == base_line) i++;
                if (j < branch_1.size()) j++;
                if (k < branch_2.size()) k++;
            }
                // If branch_1 has not changed and branch_2 has, use the line from branch_2.
            else if (branch_1_line == base_line) {
                merged.push_back(branch_2_line);
                i++;
                j++;
                if (k < branch_2.size()) k++;
            }
                // If branch_2 has not changed and branch_1 has, use the line from branch_1.
            else if (branch_2_line == base_line) {
                merged.push_back(branch_1_line);
                i++;
                if (j < branch_1.size()) j++;
                k++;
            }
                // If both branches have different lines, there is a conflict.
            else {
                *has_conflict = true;
                merged.emplace_back("<<<<<<< BRANCH 1");
                merged.push_back(branch_1_line);
                merged.emplace_back("=======");
                merged.push_back(branch_2_line);
                merged.emplace_back(">>>>>>> BRANCH 2");
                if (i < base.size()) i++;
                if (j < branch_1.size()) j++;
                if (k < branch_2.size()) k++;
            }
        }

        return merged;
    }

} // namespace manager
