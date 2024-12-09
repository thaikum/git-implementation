//
// Created by thaiku on 30/11/24.
//

#include "DirManager.h"
#include "../JitUtility/jit_utility.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <regex>
#include <utility>

namespace fs = std::filesystem;

namespace manager {

    /**
 * @brief Constructs a DirManager instance with the provided root directory.
 *
 * Initializes the DirManager with the specified root directory. This directory is used to manage the Jit repository
 * and the files within the repository.
 *
 * @param root_directory The root directory to be managed by this instance of DirManager.
 */
    DirManager::DirManager(std::string root_directory) : root_directory(std::move(root_directory)) {}

/**
 * @brief Retrieves the set of files tracked in the directory.
 *
 * This function retrieves all files in the root directory and its subdirectories that are tracked by the Jit repository.
 * It recursively scans the directory and respects any ignore rules defined in the .jitignore file.
 *
 * @return A set of strings representing the relative paths of the tracked files.
 */
    std::set<std::string> DirManager::get_files() {
        get_nested_files_in_a_directory(root_directory);
        return files;
    }

/**
 * @brief Retrieves the root directory of the managed repository.
 *
 * This function returns the root directory of the Jit repository managed by this DirManager instance.
 *
 * @return The root directory of the repository as a string.
 */
    std::string DirManager::get_root_directory() {
        return root_directory;
    }

/**
 * @brief Initializes a new Jit repository in the root directory.
 *
 * This function sets up the necessary directories and files to initialize a new Jit repository within the root directory.
 * It creates the .jit directory with subdirectories for branches, logs, refs, and objects. Additionally, it initializes
 * files like HEAD, refs/heads/master, and empty files for logs and the index.
 *
 * @throws std::runtime_error If the Jit repository already exists.
 */
    void DirManager::initialize_jit() {
        jit_directory = root_directory + "/.jit";

        if (fs::exists(jit_directory)) {
            throw std::runtime_error("Jit is already initialized for this directory");
        }

        // Create necessary Jit subdirectories.
        fs::create_directory(jit_directory);
        const std::vector<std::string> subdirs = {
                "/branches", "/logs", "/logs/refs", "/logs/refs/heads",
                "/refs", "/refs/heads", "/objects"
        };

        for (const auto &subdir: subdirs) {
            fs::create_directory(jit_directory + subdir);
        }

        // Initialize key repository files.
        const std::map<std::string, std::string> init_files = {
                {"/refs/heads/master", std::string(40, '0')},  // Master branch initialized with 40 zeros.
                {"/HEAD",              "refs/heads/master"}  // Head points to master branch.
        };

        for (const auto &[path, content]: init_files) {
            std::ofstream file(jit_directory + path);
            if (!file) {
                std::cerr << "Error creating file: " << jit_directory + path << std::endl;
            } else {
                file << content;
            }
        }

        // Create empty files for logs and index.
        const std::vector<std::string> empty_files = {"/logs/refs/heads/master", "/index"};
        for (const auto &path: empty_files) {
            std::ofstream file(jit_directory + path);
            if (!file) {
                std::cerr << "Error creating file: " << jit_directory + path << std::endl;
            }
        }

        std::cout << "Initialized empty jit repository" << std::endl;
    }

/**
 * @brief Retrieves the root directory of the Jit repository.
 *
 * This function checks if the .jit directory exists in the root directory and returns its path.
 * If the directory does not exist, it throws an error indicating that the root directory is not a Jit repository.
 *
 * @return The path to the Jit root directory.
 * @throws std::runtime_error If the directory is not a Jit repository.
 */
    std::string DirManager::get_jit_root() {
        if (fs::exists(root_directory + "/.jit")) {
            return root_directory + "/.jit";
        } else {
            throw std::runtime_error("Not a Jit repository");
        }
    }

/**
 * @brief Updates the repository by deleting and modifying files as specified.
 *
 * This function accepts two sets of modifications:
 * - Files to delete: The specified files are removed from the root directory.
 * - Files to modify: The specified files are decompressed and copied to the destination paths in the repository.
 *
 * @param files_to_delete A set of file paths to delete from the root directory.
 * @param files_to_modify A map of source file paths and their corresponding destination paths.
 */
    void DirManager::update_repository(const std::set<std::string> &files_to_delete,
                                       const std::map<std::string, std::string> &files_to_modify) {
        // Delete specified files.
        for (const auto &file: files_to_delete) {
            fs::remove(root_directory + "/" + file);
        }

        // Decompress and copy modified files to the repository.
        for (const auto &[source, destination]: files_to_modify) {
            decompress_and_copy(source, root_directory + "/" + destination);
        }
    }

/**
 * @brief Retrieves nested files in the directory, respecting ignore rules.
 *
 * This function recursively scans the directory and its subdirectories for files, adding them to the set of tracked
 * files. It reads the .jitignore file to determine which files and directories to ignore based on regex patterns.
 * Files and directories matching these patterns are excluded from being tracked.
 *
 * @param dir The directory to scan for files.
 */
    void DirManager::get_nested_files_in_a_directory(const fs::path &dir) {
        get_jit_root();  // Ensure the repository exists.

        // Read .jitignore file for patterns to ignore.
        std::ifstream jit_ignore(root_directory + "/.jitignore");
        ignore_directory_regex_construction = ".jit";
        ignore_file_names_regex_construction = "";

        if (jit_ignore) {
            std::string line;
            bool has_started = false;
            files.clear();

            // Process each line in .jitignore to build regex patterns.
            while (std::getline(jit_ignore, line)) {
                std::regex asterisk("\\*");
                std::string new_regex = std::regex_replace(line, asterisk, ".+");
                std::string prefix = has_started ? "|" : "";

                if (new_regex.back() == '/') {
                    ignore_directory_regex_construction += (prefix + new_regex);
                } else {
                    ignore_file_names_regex_construction += (prefix + new_regex);
                }
                has_started = true;
            }
        }

        // Scan the directory recursively, applying ignore rules.
        if (fs::exists(dir) && fs::is_directory(dir)) {
            std::regex ignore_files_regex(ignore_file_names_regex_construction);
            std::regex ignore_dirs_regex(ignore_directory_regex_construction);

            for (const auto &entry: fs::recursive_directory_iterator(dir)) {
                if (!entry.is_directory()) {
                    std::string path_string = entry.path().string();
                    std::string file_name = entry.path().filename().string();

                    // Skip files or directories matching ignore rules.
                    if (std::regex_search(path_string, ignore_dirs_regex) ||
                        (!ignore_file_names_regex_construction.empty() &&
                         std::regex_search(file_name, ignore_files_regex))) {
                        continue;
                    }

                    files.insert(entry.path().relative_path());
                }
            }
        }
    }

    void DirManager::change_root_directory(const std::string &root_dir) {
        root_directory = root_dir;
        jit_directory = root_dir + "/.jit/";
    }
}
