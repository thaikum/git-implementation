//
// Created by thaiku on 02/12/24.
//

#include <fstream>
#include <iostream>
#include <regex>
#include "JitActions.h"
#include "IndexFileParser.h"
#include "../CommitManagement/commit.h"
#include "../CommitManagement/CommitGraph.h"

namespace manager {

    /**
     * Constructor initializes JitActions with the root directory and validates the Jit root.
     *
     * @param root_directory The root directory of the Jit repository.
     */
    JitActions::JitActions(const std::string &root_directory) : ChangesManager(root_directory) {
    }

    /**
     * Reads and returns the current HEAD reference.
     *
     * @return The current HEAD reference.
     * @throws std::runtime_error if the HEAD file cannot be opened.
     */
    std::string JitActions::get_head() {
        std::ifstream head_file(get_jit_root() + "/HEAD");
        if (!head_file) {
            throw std::runtime_error("Could not open the HEAD file");
        }
        std::string head;
        std::getline(head_file, head);
        return head;
    }

    /**
     * Reads and returns the head of a specific branch.
     *
     * @param branch_name The name of the branch.
     * @return The head reference of the branch.
     * @throws std::runtime_error if the branch does not exist or the reference file cannot be opened.
     */
    std::string JitActions::get_branch_head(const std::string &branch_name) {
        std::ifstream head_file(get_jit_root() + "/refs/heads/" + branch_name);
        if (!head_file) {
            throw std::runtime_error("No branch named " + branch_name);
        }
        std::string head;
        std::getline(head_file, head);
        return head;
    }

    /**
     * Creates a new commit with the given message.
     *
     * @param message The commit message.
     * @throws std::runtime_error if the index file or reference files cannot be opened or written.
     */
    void JitActions::commit(const std::string &message) {
        IndexFileParser indexFileParser(get_jit_root() + "/index");
        IndexFileContent indexFileContent = indexFileParser.read_index_file();

        // Check if there are changes to commit.
        if (!indexFileContent.metaData.is_dirty) {
            std::cout << "Nothing to commit" << std::endl;
            return;
        }

        indexFileParser.prepare_commit_index_file();
        indexFileParser.write_index_file();

        std::string index_checksum = generateSHA1(get_jit_root() + "/index");
        std::string head = get_head();

        std::string old_checksum;


        if (head.starts_with("refs")) {
            std::string branch_name = std::regex_replace(head, std::regex(".+/"), "");

            old_checksum = get_branch_head(branch_name);
            update_branch_head_file(branch_name, index_checksum);
        }


        // Save the commit as a binary object and update the log.
        save_as_binary(get_jit_root() + "/objects", index_checksum, get_jit_root() + "/index");
        jit_log(get_jit_root() + "/logs/" + head, old_checksum, index_checksum, "commit: " + message);

        std::string commit_file_path = get_jit_root() + "/objects/" + generate_file_path(COMMIT_FILE_HASH).string();

        CommitGraph commit_graph(commit_file_path);

        Commit commit;
        commit.checksum = index_checksum;
        commit.message = message;
        commit.timestamp = std::chrono::system_clock::now();
        commit.branch_name = head.starts_with("refs") ? std::regex_replace(head, std::regex(".+/"), "") :
                             commit_graph.get_commit(old_checksum) !=
                             nullptr ? commit_graph.get_commit(old_checksum)->branch_name : "wild";

        commit_graph.add_commit(commit, {old_checksum});
        commit_graph.save_commits(commit_file_path);

        if (head.starts_with("refs")) {
            update_head_file(head);
        } else {
            update_head_file(index_checksum);
        }
    }

    /**
     * Updates the HEAD file with a new reference.
     *
     * @param head The new HEAD reference to set.
     * @throws std::runtime_error if the HEAD file cannot be opened.
     */
    void JitActions::update_head_file(const std::string &head) {
        std::ofstream head_file(get_jit_root() + "/HEAD");
        if (!head_file) {
            throw std::runtime_error("Head file could not be opened");
        }
        head_file << fs::path(head).lexically_normal().string();
    }

    void JitActions::update_branch_head_file(const std::string &branch_name, const std::string &checksum) {
        std::ofstream head_file(get_jit_root() + "/refs/heads/" + branch_name);
        head_file << checksum;
        head_file.close();
    };

    /**
     * Creates a new branch with the given name.
     *
     * @param branch_name The name of the new branch.
     * @throws std::runtime_error if the branch name is invalid or any files cannot be opened or written.
     */
    void JitActions::create_branch(const std::string &branch_name) {
        std::regex branch_name_validator("^[A-Za-z0-9\\._\\-]+$");
        if (!std::regex_match(branch_name, branch_name_validator)) {
            throw std::runtime_error(
                    "Branch name can only contain alphanumeric characters, underscores, and hyphens.");
        }

        throw_error_if_repo_is_dirty();

        std::ifstream head_file(get_jit_root() + "/HEAD");
        if (!head_file) {
            throw std::runtime_error("Could not open the HEAD file");
        }

        std::string head;
        std::getline(head_file, head);

        // Extract reference if HEAD points to a branch.
        if (head.starts_with("refs")) {
            std::ifstream reference_file(get_jit_root() + "/" + head);
            if (!reference_file) {
                throw std::runtime_error("Could not open the reference file");
            }
            std::getline(reference_file, head);
        }

        std::string new_branch_head = "refs/heads/" + branch_name;

        // Update HEAD and create the branch.
        update_head_file(new_branch_head);

        std::ofstream branch_head_file(get_jit_root() + "/" + new_branch_head);
        if (!branch_head_file) {
            throw std::runtime_error("Failed to create a head file for branch: " + branch_name);
        }
        branch_head_file << head;

        jit_log(get_jit_root() + "/logs/" + new_branch_head, head, head, "branch: " + branch_name);
    }

//    void JitActions::update_branch_and_head_after_branch_creation()

    /**
     * Checks out to a specific commit or branch.
     *
     * @param target The target commit hash or branch name to checkout to.
     * @throws std::runtime_error if the target commit or branch cannot be found.
     */
    void JitActions::checkout_to_a_commit(const std::string &target) {
        fs::path objects_path = fs::path(get_jit_root() + "/objects");
        fs::path path = objects_path / generate_file_path(target);
        std::string current_head = target;

        // Check if target is a branch.
        if (!fs::exists(path)) {
            std::ifstream branch_file(get_jit_root() + "/refs/heads/" + target);
            if (branch_file) {
                std::string branch_head;
                std::getline(branch_file, branch_head);
                path = objects_path / generate_file_path(branch_head);
                current_head = "refs/heads/" + target;
            } else {
                throw std::runtime_error("Target " + target + " was not found!");
            }
        }

        throw_error_if_repo_is_dirty();

        // Perform checkout if target exists.
        if (fs::exists(path)) {
            decompress_and_copy(path, get_jit_root() + "/index");

            IndexFileParser indexFileParser(get_jit_root() + "/index");
            IndexFileContent content = indexFileParser.read_index_file();

            checkout(content);

            std::cout << "Head now at " << target << std::endl;
            update_head_file(current_head);
        } else {
            throw std::runtime_error("No branch or commit matches " + target);
        }
    }

    void JitActions::checkout(const IndexFileContent &content) {

        std::set<std::string> current_files = transform_file_names();
        std::map<std::string, std::string> files_to_replace;
        fs::path objects_path(get_jit_root() + "/objects");

        for (const auto &[_, file_info]: content.files_map) {
            current_files.erase(file_info.filename);
            files_to_replace.emplace(
                    objects_path / generate_file_path(file_info.checksum), file_info.filename);
        }

        update_repository(current_files, files_to_replace);
    }

    /**
     * Prints the commit log for the current branch.
     *
     * @throws std::runtime_error if the HEAD file cannot be opened.
     */
    void JitActions::jit_commit_log() {
        std::ifstream head_file(get_jit_root() + "/HEAD");
        if (!head_file) {
            throw std::runtime_error("Could not open the HEAD file");
        }

        std::string head;
        std::getline(head_file, head);

        std::string commit_file_path = get_jit_root() + "/objects/" + generate_file_path(COMMIT_FILE_HASH).string();
        CommitGraph commitGraph(commit_file_path);

        if (head.starts_with("refs")) {
            head = get_branch_head(std::regex_replace(head, std::regex(".+/"), ""));
        }

        commitGraph.print_commit_history(head);

//        print_commit_log(get_jit_root() + "/logs/" + head);
    }

    /**
     * Retrieves the list of all branches in the repository.
     *
     * @return A set of all branch names.
     */
    std::set<std::string> JitActions::get_branches() {
        std::set<std::string> branches;
        for (const auto &branch_head: fs::directory_iterator(get_jit_root() + "/refs/heads")) {
            branches.insert(branch_head.path().filename().string());
        }
        return branches;
    }

    /**
     * Lists all branches, highlighting the current branch.
     *
     * @throws std::runtime_error if the HEAD file cannot be opened.
     */
    void JitActions::list_jit_branches() {
        std::string cur_branch = get_head();
        auto all_branches = get_branches();

        if (cur_branch.starts_with("refs")) {
            cur_branch = std::regex_replace(cur_branch, std::regex(".+/"), "");
        }

        for (const auto &branch: all_branches) {
            if (branch == cur_branch) {
                std::cout << "* " << GREEN << branch << RESET << std::endl;
            } else {
                std::cout << "  " << branch << std::endl;
            }
        }
    }
}
