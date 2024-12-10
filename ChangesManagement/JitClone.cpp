//
// Created by thaiku on 08/12/24.
//

#include <stack>
#include <iostream>
#include <fstream>
#include <regex>
#include "JitActions.h"

namespace manager {
    void JitActions::jit_clone(const std::string &repository_dir) {
        jit_clone(repository_dir, fs::path("./" + repository_dir).filename());
    }

    void JitActions::jit_clone(const std::string &repository_dir, const std::string &target_dir) {
        if (!fs::exists(target_dir))
            fs::create_directories(target_dir+"/.jit");

        if (fs::is_directory(target_dir)) {
            if (fs::is_directory(repository_dir) && fs::is_directory(repository_dir + "/.jit")) {
                fs::path repo_pat(repository_dir);
                change_root_directory(repository_dir);
                fs::create_directories(target_dir + "/.jit");

                for(const auto &file : fs::recursive_directory_iterator(repository_dir + "/.jit")){
                    if(!file.is_directory()){
                        fs::path target_file = fs::path(target_dir)/ file.path().lexically_relative(repository_dir);
                        copy_file(file.path(), target_file);
                    }
                }

                change_root_directory(absolute(fs::path(target_dir)));
                std::string head = std::regex_replace(get_head(), std::regex(".+/"), "");
                std::string head_commit = get_branch_head(head);
                auto content = IndexFileParser::read_binary_index_file(
                        target_dir +"/.jit/objects/"+ generate_file_path(head_commit).string());
                checkout(content);
            } else {
                throw std::runtime_error("No Jit repository named " + repository_dir + " was found.");
            }
        } else {
            throw std::runtime_error("Invalid target directory");
        }
    }

    void JitActions::jit_branch_clone(const std::string &branch_name, const std::string &repository_dir, int depth) {
        jit_branch_clone(branch_name, repository_dir, fs::path("./" + repository_dir).filename(), depth);
    }

    void JitActions::copy_file(const std::string &source_path, const std::string &destination_path) {
        try {
            // Extract the directory from the destination path
            fs::path destination_dir = fs::path(destination_path).parent_path();

            // Create any missing directories in the destination path
            if (!fs::exists(destination_dir)) {
                fs::create_directories(destination_dir);
            }

            //(overwrite if exists)
            fs::copy(source_path, destination_path, fs::copy_options::overwrite_existing);
        } catch (const fs::filesystem_error &e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    void JitActions::jit_branch_clone(const std::string &branch_name, const std::string &repository_dir,
                                      const std::string &target_dir, int depth) {

        fs::create_directory(target_dir);
        if (fs::is_directory(target_dir)) {
            if (fs::is_directory(repository_dir) && fs::is_directory(repository_dir + "/.jit")) {
                fs::path repo_pat(repository_dir);
                change_root_directory(repository_dir);

                std::stack<std::string> branch_commits = get_commit_stack(
                        repository_dir + "/.jit/logs/refs/heads/" + branch_name);

                std::unique_ptr<IndexFileContent> latest_content = nullptr;
                while ((depth == -1 && !branch_commits.empty()) || depth > 0) {
                    std::string commit = branch_commits.top();
                    IndexFileContent content = IndexFileParser::read_binary_index_file((get_jit_root() + "/objects/" +
                                                                                        generate_file_path(
                                                                                                commit).string()));


                    if (latest_content == nullptr) {
                        latest_content = std::make_unique<IndexFileContent>(content);
                    }

                    for (const auto &[_, info]: content.files_map) {
                        std::string file_dir = generate_file_path(info.checksum).string();
                        std::string result_path = target_dir + "/.jit/objects/" + file_dir;

                        copy_file(get_jit_root() + "/objects/" + file_dir,
                                  result_path);
                    }
//
                    //copy index file
                    copy_file((get_jit_root() + "/objects/" +
                               generate_file_path(commit).string()),
                              (target_dir + "/.jit/objects/" +
                               generate_file_path(commit).string()));
//
                    //copy the commit graph
                    copy_file((get_jit_root() + "/objects/" +
                               generate_file_path(COMMIT_FILE_HASH).string()),
                              (target_dir + "/.jit/objects/" +
                               generate_file_path(COMMIT_FILE_HASH).string()));

                    branch_commits.pop();
                    depth--;
                }

                //copy the branch head
                copy_file(get_jit_root() + "/refs/heads/" + branch_name,
                          target_dir + "/.jit/refs/heads/" + branch_name);

                //copy the branch log
                copy_file(get_jit_root() + "/logs/refs/heads/" + branch_name,
                          target_dir + "/.jit/logs/refs/heads/" + branch_name);

                //copy index file
                decompress_and_copy((get_jit_root() + "/objects/" +
                                     generate_file_path(get_branch_head(branch_name)).string()),
                                    (target_dir + "/.jit/index"));

                std::ofstream head(target_dir + "/.jit/HEAD");
                if (head) {
                    head << "refs/heads/" << branch_name << std::endl;
                    head.close();
                } else {
                    throw std::runtime_error("Heads file failed to be created");
                }

                change_root_directory(target_dir);

                checkout(*latest_content);
                std::cout << "Clone successful" << std::endl;
            } else {
                throw std::runtime_error("No Jit repository named " + repository_dir + " was found.");
            }
        } else {
            throw std::runtime_error("Invalid target directory");
        }
    }
}