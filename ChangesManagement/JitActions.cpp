//
// Created by thaiku on 02/12/24.
//

#include <fstream>
#include <iostream>
#include <regex>
#include "JitActions.h"
#include "../JitUtility/jit_utility.h"
#include "IndexFileParser.h"

namespace manager {
    JitActions::JitActions(std::string jit_root) : jit_root(std::move(jit_root)), ChangesManager(
            std::regex_replace(jit_root, std::regex(".jit"), "")) {}

    std::string JitActions::get_head() {
        std::ifstream head_file(jit_root + "/HEAD");
        if (head_file) {
            std::string head;
            getline(head_file, head);
            head_file.close();
            return head;
        } else {
            throw std::runtime_error("Could not open the HEAD file");
        }
    }

    std::string JitActions::get_branch_head(const std::string &branch_name) {
        std::ifstream head_file(jit_root+"/refs/heads/"+ branch_name);

        if (head_file) {
            std::string head;
            getline(head_file, head);
            head_file.close();
            return head;
        } else {
            throw std::runtime_error("Could not open the HEAD file");
        }
    }

    void JitActions::commit(const std::string &message) {
        IndexFileParser indexFileParser(jit_root + "/index");
        IndexFileContent indexFileContent = indexFileParser.read_index_file();

        if (!indexFileContent.metaData.is_dirty) {
            std::cout << "Nothing to commit";
            return;
        }

        indexFileParser.prepare_commit_index_file();
        indexFileParser.write_index_file();

        std::string index_checksum = generateSHA1(jit_root + "/index");

        std::string head = get_head();

        std::fstream reference_file(jit_root + "/" + head, std::ios::in | std::ios::out);
        if (!reference_file) {
            throw std::runtime_error("Could not open the reference file");
        }

        std::string old_checksum;
        getline(reference_file, old_checksum);

        if (reference_file) {
            reference_file.clear();
            reference_file.seekp(0, std::ios::beg);
            reference_file << index_checksum;
            reference_file.close();

            save_as_binary(jit_root + "/objects", index_checksum, jit_root + "/index");
            jit_log(jit_root + "/logs/" + head, old_checksum, index_checksum, "commit: " + message);
        } else {
            throw std::runtime_error("Failed to read old checksum from the reference file");
        }
    }

    void JitActions::update_head_file(const std::string &head) {
        std::ofstream head_file(jit_root + "/HEAD");

        if (head_file) {
            head_file << head;
        } else {
            throw std::runtime_error("Head file could not be opened");
        }

    }

    void JitActions::create_branch(const std::string &branch_name) {
        //todo validate a branch name
        //todo ensure that the repository is not dirty
        std::ifstream head_file(jit_root + "/HEAD");
        if (head_file) {
            std::string head;
            getline(head_file, head);
            head_file.close();
            std::string current_head;

            //extract the head if it is a reference
            if (head.starts_with("refs")) {
                std::fstream reference_file(jit_root + "/" + head, std::ios::in | std::ios::out);
                if (!reference_file) {
                    throw std::runtime_error("Could not open the reference file");
                }
                getline(reference_file, head);
                reference_file.close();
            }

            std::string new_branch_head = "/refs/heads/" + branch_name;


            //update_head
            update_head_file(new_branch_head);

            std::ofstream branch_head_file(jit_root + new_branch_head);

            if (branch_head_file) {
                branch_head_file << head;
                jit_log(jit_root + "/logs/" + new_branch_head, head, head, "branch: " + branch_name);
            } else {
                throw std::runtime_error("Failed to create a head file for branch: " + branch_name);
            }
        }

    }

    void JitActions::checkout_to_a_branch(const std::string &branch_name) {

    }

    void JitActions::checkout_to_a_commit(const std::string &target) {
        fs::path objects_path = fs::path(jit_root + "/objects");
        fs::path path = objects_path / generate_file_path(target);
        std::string current_head = target;

        if (!fs::exists(path)) {
            std::ifstream branch_file(jit_root + "/refs/heads/" + target);
            if (branch_file) {
                std::string branch_head;
                getline(branch_file, branch_head);
                path = objects_path / generate_file_path(branch_head);
                current_head = "refs/heads/" + target;
            } else {
                throw std::runtime_error("Target " + target + " was not found!");
            }
        }


        if (fs::exists(path)) {
            std::ifstream index(path);

            if (index) {
                decompress_and_copy(path, (jit_root + "/index"));
                IndexFileParser indexFileParser(jit_root + "/index");
                IndexFileContent content = indexFileParser.read_index_file();

                std::set<std::string> current_files = transform_file_names();
                std::map<std::string, std::string> files_to_replace;

                for (auto &file_info: content.files) {
                    if (current_files.contains(file_info.filename)) {}
                    current_files.erase(file_info.filename);

                    files_to_replace.insert(
                            std::pair((objects_path / generate_file_path(file_info.checksum)), file_info.filename));
                }

                update_repository(current_files, files_to_replace);
                std::cout << "Head now at " << target << std::endl;
                update_head_file(current_head);
            } else {
                throw std::runtime_error("No branch nor commit matches " + target);
            }
        }
    }

    void JitActions::jit_commit_log() {
        std::ifstream head_file(jit_root + "/HEAD");
        if (head_file) {
            std::string head;
            getline(head_file, head);
            head_file.close();

            print_commit_log(jit_root + "/logs/" + head);
        }
    }
}