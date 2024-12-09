//
// Created by thaiku on 02/12/24.
//

#ifndef JIT_JITACTIONS_H
#define JIT_JITACTIONS_H

#include <string>
#include <stack>
#include <unordered_set>
#include "../JitUtility/jit_utility.h"
#include "ChangesManager.h"


namespace fs = std::filesystem;

namespace manager {

    /**
     * @class JitActions
     * @brief Provides various actions related to managing a Git-like repository (commit, checkout, branch management, etc.).
     *
     * This class encapsulates functionality for handling Git-like operations such as committing changes,
     * viewing logs, creating branches, and checking out commits. It is built on top of the ChangesManager class
     * to facilitate changes tracking in a repository. Operations are performed using file manipulation and
     * custom checksums.
     */
    class JitActions : public ChangesManager {
    public:
        /**
         * @brief Constructs a JitActions object with the specified repository root directory.
         *
         * Initializes the repository by determining the Jit root directory and validating it.
         *
         * @param jit_root The root directory of the Jit repository.
         */
        explicit JitActions(const std::string &jit_root);

        /**
         * @brief Commits changes to the repository with a given commit message.
         *
         * This function checks if there are any uncommitted changes, prepares the index file, and commits the
         * changes by updating the commit logs and creating a new commit object.
         *
         * @param message The commit message to associate with the commit.
         */
        void commit(const std::string &message);

        /**
         * @brief Prints the commit log of the current branch.
         *
         * Displays the list of commits made on the current branch, starting from the most recent one.
         */
        void jit_commit_log();

        /**
         * @brief Checks out a specific commit or branch.
         *
         * If the target is a commit, it checks out the commit. If the target is a branch, it checks out the head
         * of that branch.
         *
         * @param target The commit hash or branch name to check out.
         */
        void checkout_to_a_commit(const std::string &target);

        /**
         * @brief Retrieves a map of changed files' content between the current repository state and the specified
         *        commit or branch.
         *
         * @return A map of file names and their respective content from the changed files.
         */
        std::map<std::string, std::vector<std::string>> get_changed_files_content();

        /**
         * @brief Creates a new branch with the specified name.
         *
         * This method validates the branch name, ensures the repository is clean (no uncommitted changes),
         * and creates a new branch by updating the HEAD reference.
         *
         * @param branch_name The name of the branch to create.
         */
        void create_branch(const std::string &branch_name);

        /**
         * @brief Merges the specified feature branch into the current branch.
         *
         * This function attempts to merge changes from the given branch into the current branch.
         *
         * @param feature_branch The branch to merge into the current branch.
         */
        void merge(const std::string &feature_branch);

        /**
         * @brief Displays the difference between the current repository state and the HEAD of the repository.
         */
        void jit_diff();

        /**
         * @brief Lists all the branches in the repository and highlights the current branch.
         */
        void list_jit_branches();

        /**
         * @brief Displays the difference between the current branch and a specified branch.
         *
         * @param branch_name The name of the branch to compare against.
         */
        void jit_diff(const std::string &branch_name);

        void jit_clone(const std::string &repository_dir);

        void jit_clone(const std::string &repository_dir, const std::string &target_dir);

        void jit_branch_clone(const std::string &branch_name, const std::string &repository_dir, int depth);

        void jit_branch_clone(const std::string &branch_name, const std::string &repository_dir,
                              const std::string &target_dir, int depth);

    private:
//        std::string jit_root; ///< The root directory of the Jit repository.

        /**
         * @brief Updates the HEAD reference file with a new commit or branch reference.
         *
         * This function writes a new reference (commit or branch) into the HEAD file, pointing to the latest
         * commit or branch.
         *
         * @param head The new reference to set in the HEAD file.
         */
        void update_head_file(const std::string &head);

        /**
         * @brief Retrieves the current commit reference stored in the HEAD file.
         *
         * @return A string representing the reference of the current HEAD.
         */
        [[nodiscard]] std::string get_head();

        /**
         * @brief Retrieves the HEAD reference for a specific branch.
         *
         * @param branch_name The name of the branch to retrieve the HEAD reference for.
         * @return The HEAD reference of the specified branch.
         */
        [[nodiscard]] std::string get_branch_head(const std::string &branch_name);

        /**
         * @brief Creates a temporary file with the specified checksum as its content.
         *
         * @param checksum The checksum to create the temporary file with.
         * @return The path to the temporary file.
         */
        [[nodiscard]] std::string create_temp_file(const std::string &checksum);

        /**
         * @brief Reads the content of a file and returns it as a vector of strings.
         *
         * @param filename The name of the file to read.
         * @return A vector of strings containing the content of the file.
         */
        [[nodiscard]] static std::vector<std::string> read_file_to_vector(const std::string &filename);

        /**
         * @brief Retrieves the set of all branches in the repository.
         *
         * @return A set of branch names.
         */
        [[nodiscard]] std::set<std::string> get_branches();

        /**
         * @brief Retrieves the intersection commit shared between two branches.
         *
         * This function calculates the common commit point between two branches in the repository.
         *
         * @param branch1 The name of the first branch.
         * @param branch2 The name of the second branch.
         * @return The commit reference representing the intersection.
         */
        std::string get_intersection_commit(const std::string &branch1, const std::string &branch2);

        /**
         * @brief Retrieves the changed files' content between two branches.
         *
         * This function compares the file content between two branches, returning the changes made.
         *
         * @param map1 The file map of the first branch.
         * @param map2 The file map of the second branch.
         * @return A map of file names with changes detected between the branches.
         */
        std::unordered_map<std::string, std::pair<std::vector<std::string>, std::vector<std::string>>>
        get_changed_files_data(const std::unordered_map<std::string, FileInfo> &map1, std::unordered_map<std::string, FileInfo> map2);

        /**
         * @brief Displays the difference between two branches.
         *
         * This function computes and displays the differences between two branches based on their file changes.
         *
         * @param branch1 The name of the first branch.
         * @param branch2 The name of the second branch.
         */
        void jit_diff(const std::string &branch1, const std::string &branch2);

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
        static std::vector<std::string>
        three_way_merge(const std::vector<std::string> &base, const std::vector<std::string> &branch_1,
                        const std::vector<std::string> &branch_2,
                        const std::shared_ptr<bool>& has_conflicts);


        /**
         * @brief Writes the contents of a vector of strings to a file, each string on a new line.
         *
         * This function takes a vector of strings and writes each string to a file. The strings are written to
         * the file with a newline separating them. If the file already exists, it will be overwritten.
         *
         * @param filename The name of the file to write the data to.
         * @param lines A vector of strings to be written to the file, each string will appear on its own line.
         */
        static void write_vector_to_file(const std::string &filename, const std::vector<std::string> &lines);

        static std::stack<std::string> get_commit_stack(const std::string &file_name);

        void checkout(const IndexFileContent &content);

        static void copy_file(const std::string &source_path, const std::string &destination_path);

        void update_branch_head_file(const std::string &branch_name, const std::string &checksum);
    };

} // namespace manager

#endif //JIT_JITACTIONS_H
