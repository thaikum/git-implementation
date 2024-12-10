/**
 * @file DirManager.h
 * @brief Declares the DirManager class for managing a Jit repository in a given directory.
 *
 * This file defines the `DirManager` class, which provides functionality for managing a Jit repository in a specified
 * root directory. The class allows for initializing a new repository, retrieving tracked files, updating the repository,
 * and handling nested files in the directory while respecting `.jitignore` patterns.
 */

#ifndef JIT_DIRMANAGER_H
#define JIT_DIRMANAGER_H

#include <set>
#include <string>
#include <filesystem>
#include <map>

namespace manager {

    /**
     * @class DirManager
     * @brief Manages a Jit repository in the specified root directory.
     *
     * The `DirManager` class provides methods for initializing a Jit repository, retrieving tracked files, and updating
     * the repository. It also provides functionality for recursively scanning directories and respecting ignore rules
     * defined in the `.jitignore` file.
     */
    class DirManager {
    public:
        /**
         * @brief Constructs a DirManager instance with the specified root directory.
         *
         * This constructor initializes a `DirManager` instance, managing the directory where the Jit repository is
         * located or will be initialized.
         *
         * @param root_directory The root directory to manage. This directory will house the Jit repository.
         */
        explicit DirManager(std::string root_directory);

        /**
         * @brief Retrieves the set of files tracked by the Jit repository in the root directory.
         *
         * This function scans the root directory and returns a set of file paths that are tracked by the Jit repository.
         * It respects the `.jitignore` file to avoid tracking ignored files or directories.
         *
         * @return A set of relative file paths that are tracked by the repository.
         */
        [[nodiscard]] std::set<std::string> get_files();

        /**
         * @brief Initializes a new Jit repository in the root directory.
         *
         * This function initializes a new Jit repository in the specified root directory by creating necessary subdirectories
         * and key files such as logs, branches, refs, and objects. It ensures the repository is correctly structured and
         * prevents initializing if the repository already exists.
         *
         * @throws std::runtime_error If the Jit repository already exists in the root directory.
         */
        void initialize_jit();

        /**
         * @brief Retrieves the root directory of the managed repository.
         *
         * This function returns the root directory of the repository managed by the `DirManager` instance.
         *
         * @return The root directory as a string.
         */
        [[nodiscard]] std::string get_root_directory();

        /**
         * @brief Retrieves the root directory of the Jit repository.
         *
         * This function checks if the `.jit` directory exists within the root directory and returns its path.
         * If the repository does not exist, it throws a runtime error.
         *
         * @return The path to the `.jit` directory within the root directory.
         * @throws std::runtime_error If the root directory does not contain a Jit repository.
         */
        std::string get_jit_root();

    private:
        std::string root_directory; /**< The root directory of the repository. */
        std::string jit_directory;  /**< The directory where the Jit repository is initialized (i.e., `.jit`). */
        std::set<std::string> files; /**< A set of files tracked by the Jit repository. */
        std::string ignore_file_names_regex_construction; /**< Regex string for ignoring specific file names. */
        std::string ignore_directory_regex_construction; /**< Regex string for ignoring specific directories. */

    protected:
        /**
         * @brief Scans the directory recursively for files, respecting `.jitignore` rules.
         *
         * This function scans the specified directory and its subdirectories to collect file paths. It uses the patterns
         * defined in the `.jitignore` file to exclude files and directories that match the ignore rules.
         *
         * @param dir The directory to scan recursively for tracked files.
         */
        void get_nested_files_in_a_directory(const std::filesystem::path &dir);

        /**
         * @brief Updates the repository by deleting and modifying files.
         *
         * This function performs updates to the Jit repository by:
         * 1. Deleting the files listed in the `files_to_delete` set.
         * 2. Modifying or adding files by decompressing the specified source files and copying them to the target
         *    destination paths.
         *
         * @param files_to_delete A set of file paths to delete from the root directory.
         * @param files_to_modify A map of source file paths and corresponding destination paths in the repository.
         */
        void update_repository(const std::set<std::string> &files_to_delete,
                               const std::map<std::string, std::string> &files_to_modify);

        void change_root_directory(const std::string &root_dir);

    };

} // namespace manager

#endif // JIT_DIRMANAGER_H
