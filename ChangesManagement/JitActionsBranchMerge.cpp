//
// Created by thaiku on 04/12/24.
//

#include <fstream>
#include <regex>
#include <future>
#include <iostream>
#include "JitActions.h"
#include "../JitUtility/jit_utility.h"

namespace manager {
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
            throw std::runtime_error("Failed to open file:  " + file_name);
        }

    }

    void JitActions::merge(const std::string &feature_branch) {
        std::string head = get_head();

        if (head.starts_with("refs")) {
            std::string branch_name = std::regex_replace(head, std::regex(".+/"), "");

            std::future<std::string> feature_branch_head = std::async(std::launch::async, [this, feature_branch] {
                return get_branch_head(feature_branch);
            });

            auto base_branch_commit_tree = get_commit_tree(jit_root + "/logs/" + head);
            auto feature_branch_commit_tree = get_commit_tree(jit_root + "/logs/refs/heads/" + feature_branch);
            std::string base_commit;

            for (auto &branch_pair: feature_branch_commit_tree) {
                if (base_branch_commit_tree.contains(branch_pair.first)) {
                    base_commit = branch_pair.first;
                    break;
                }
            }

            if (!base_commit.empty()) {
                fs::create_directory(jit_root + "/temp");

                std::string temp_feature_branch_file = jit_root + "/temp/" + feature_branch;
                std::string temp_base_commit_file = jit_root + "/temp/base";

                fs::path base_index_file = fs::path(jit_root + "/objects") / generate_file_path(base_commit);
                fs::path feature_branch_index_file =
                        fs::path(fs::path(jit_root + "/objects")) / generate_file_path(feature_branch_head.get());


                decompress_and_copy(base_index_file, temp_base_commit_file);
                decompress_and_copy(feature_branch_index_file, temp_feature_branch_file);

                IndexFileContent base_content = IndexFileParser(temp_base_commit_file).read_index_file();
                IndexFileContent feature_branch_content = IndexFileParser(temp_feature_branch_file).read_index_file();
                IndexFileContent main_branch = IndexFileParser(jit_root + "/index").read_index_file();

                auto f_branch = feature_branch_content.files_map;
                auto b_branch = base_content.files_map;

                std::vector<FileInfo> merged_files;
                std::map<std::string, FileInfo> merged_files_map;

                for (auto &main_pair: main_branch.files_map) {
                    std::string absolute_path = get_root_directory() + "/" + main_pair.first;

                    if (b_branch.contains(main_pair.first)) {
                        const auto &base = b_branch.at(main_pair.first);
                        bool file_in_f = f_branch.contains(main_pair.first);

                        if ((base.checksum == main_pair.second.checksum)) {
                            std::cout << "Found " << main_pair.first << std::endl;

                            //file modified by the feature branch
                            if (file_in_f && f_branch.at(main_pair.first).checksum != main_pair.second.checksum) {
                                auto ff = f_branch.at(main_pair.first);
                                decompress_and_copy((jit_root + "/objects/" + generate_file_path(ff.checksum).string()),
                                                    absolute_path);
                                merged_files.push_back(ff);
                                merged_files_map.insert({ff.filename, std::move(ff)});
                            } else {
                                merged_files.push_back(main_pair.second);
                                merged_files_map.insert(main_pair);
                            }

                            f_branch.erase(main_pair.first);

                            for (auto k: f_branch) {
                                std::cout << k.first << std::endl;
                            }
                        } else {
                            std::cout << "Found 2 " << main_pair.first << std::endl;
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
                                                                                               main_file_future.get());
                                write_vector_to_file(absolute_path, merged_vector);
                                auto sha = generateSHA1(absolute_path);

                                main_pair.second.is_dirty = true;
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
                        std::cout << "Present in both but not in master" << std::endl;
                        //file present in both branches but absent in base
                        std::string temp_branch_file = create_temp_file(f_branch.at(main_pair.first).checksum);
                        const auto branch_file_vector = read_file_to_vector(temp_branch_file);
                        const auto main_file_vector = read_file_to_vector(absolute_path);
                        const std::vector<std::string> base;

                        std::cout << "First " << branch_file_vector.at(0) << std::endl;
                        std::cout << "Main " << branch_file_vector.at(0) << std::endl;

                        const auto merged_vector = three_way_merge(base, branch_file_vector, main_file_vector);
                        write_vector_to_file(absolute_path, merged_vector);
                        f_branch.erase(main_pair.first);
                        //todo handle cases of conflict
                    } else {
                        merged_files.push_back(main_pair.second);
                        merged_files_map.insert(main_pair);
                    }
                }

                for (auto &f_pair: f_branch) {
                    std::string absolute_path = get_root_directory() + "/" + f_pair.first;
                    std::cout << "File name is: " << f_pair.first << " Checksum is " << f_pair.second.checksum
                              << std::endl;
                    decompress_and_copy(
                            (jit_root + "/objects/" + generate_file_path(f_pair.second.checksum).string()),
                            absolute_path);
                    merged_files.push_back(f_pair.second);
                    merged_files_map.insert({f_pair.second.filename, std::move(f_pair.second)});
                }

                //delete the temporary directory
                fs::remove_all(jit_root + "/temp/");
                std::cout << "Merge completed KUDOS!";
            } else {
                throw std::runtime_error("The branches are not related! Orphan merge out of scope");
            }
        } else {
            throw std::runtime_error("Cannot perform merge while outside a branch");
        }
    }

    std::string JitActions::create_temp_file(const std::string &checksum) {
        std::string new_file_name = jit_root + "/temp/" + checksum;
        decompress_and_copy((jit_root + "/objects/" + generate_file_path(checksum).string()), new_file_name);
        return new_file_name;
    }

    std::string JitActions::create_temp_file(const std::string &checksum, const std::string& file_name) {
        std::string new_file_name = jit_root + "/temp/" + file_name;
        decompress_and_copy((jit_root + "/objects/" + generate_file_path(checksum).string()), new_file_name);
        return new_file_name;
    }

    std::vector<std::string> JitActions::read_file_to_vector(const std::string &filename) {
        std::vector<std::string> lines;
        std::ifstream file(filename);

        if (file) {
            std::string line;
            while (std::getline(file, line)) {
                lines.push_back(line);
            }
            return lines;
        } else {
            throw std::runtime_error("Could not open " + filename);
        }
    }

    void JitActions::write_vector_to_file(const std::string &filename, const std::vector<std::string> &lines) {
        std::ofstream file(filename);

        for (auto &line: lines) {
            file << line << std::endl;
        }
        file.close();
    }

    std::vector<std::string> JitActions::three_way_merge(
            const std::vector<std::string> &base,
            const std::vector<std::string> &branch_1,
            const std::vector<std::string> &branch_2) {

        std::vector<std::string> merged;
        size_t i = 0, j = 0, k = 0;

        while (i < base.size() || j < branch_1.size() || k < branch_2.size()) {
            std::string base_line = (i < base.size()) ? base[i] : "";
            std::string branch_1_line = (j < branch_1.size()) ? branch_1[j] : "";
            std::string branch_2_line = (k < branch_2.size()) ? branch_2[k] : "";

            if (branch_1_line == branch_2_line) {
                std::cout << "Attempting merging" << std::endl;

                merged.push_back(branch_1_line);
                if (i < base.size() && branch_1_line == base_line) i++;
                if (j < branch_1.size()) j++;
                if (k < branch_2.size()) k++;
            } else if (branch_1_line == base_line) {
                // Only branch_2 changed the line
                merged.push_back(branch_2_line);
                i++;
                j++;
                if (k < branch_2.size()) k++;
            } else if (branch_2_line == base_line) {
                // Only branch_1 changed the line
                merged.push_back(branch_1_line);
                i++;
                if (j < branch_1.size()) j++;
                k++;
            } else {
                std::cout << "Here is conflict" << std::endl;
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

}