//
// Created by thaiku on 30/11/24.
//

#include "ChangesManager.h"
#include "data.h"
#include "IndexFileParser.h"
#include "../JitUtility/jit_utility.h"

#include <iostream>
#include <fstream>
#include <regex>
#include <ranges>
#include <set>

namespace fs = std::filesystem;

namespace manager {

    /**
     * Constructor that initializes the DirManager and sets the root directory.
     * @param root_directory The root directory of the repository.
     */
    ChangesManager::ChangesManager(const std::string &root_directory)
            : DirManager(root_directory){
    }

    /**
     * Generates a map of files with their metadata (e.g., checksum, modification date).
     *
     * @param files_to_add A set of file names to be processed and added to the map.
     * @return A map of filenames to FileInfo objects, each containing metadata of the corresponding file.
     * @throws std::runtime_error if any file in files_to_add cannot be found or read.
     */
    std::unordered_map<std::string, FileInfo> ChangesManager::get_files_map(const std::set<std::string> &files_to_add) {
        std::unordered_map<std::string, FileInfo> current_files;

        for (const auto &file_name : files_to_add) {
            FileInfo file_info;
            file_info.filename = file_name;
            file_info.checksum = generateSHA1(get_root_directory() + "/" + file_name);
            file_info.is_dirty = false;
            file_info.is_new = false;

            // Convert filesystem time to system_clock time
            try {
                auto file_time = fs::last_write_time(get_root_directory() + "/" + file_name);
                file_info.last_modified = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                        file_time - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
                );
            } catch (const std::exception &e) {
                throw std::runtime_error("Error reading file time for " + file_name);
            }

            current_files[file_info.filename] = file_info;
        }

        return current_files;
    }

    /**
     * Transforms file names by removing directory structure and leading slashes/dots.
     *
     * @return A set of transformed file names without directories.
     */
    std::set<std::string> ChangesManager::transform_file_names() {
        auto temp_files = get_files();

        // Use range-based transformation to clean up file names
        auto transformed_filenames = std::views::transform(temp_files, [](const std::string &file_name) {
            return fs::path(file_name).lexically_normal().string();
        });

        files = std::set(transformed_filenames.begin(), transformed_filenames.end());
        return files;
    }

    /**
     * Checks if the repository has uncommitted changes and throws an error if it does.
     *
     * @throws std::runtime_error if the repository has uncommitted changes.
     */
    void ChangesManager::throw_error_if_repo_is_dirty() {
        transform_file_names();
        auto file_map = get_files_map(files);
        JitStatus status;
        IndexFileParser parser(file_map, get_jit_root() + "/index");
        IndexFileContent previous_content = parser.read_index_file();

        for (auto const &file_info_pair : file_map) {
            auto file_name = file_info_pair.first;
            auto file_info = file_info_pair.second;

            if (previous_content.files_map.contains(file_name)) {
                auto old_file_info = previous_content.files_map.at(file_name);
                if (old_file_info.checksum != file_info.checksum) {
                    throw std::runtime_error("You have uncommitted changes! Please commit them first");
                } else {
                    if (old_file_info.is_dirty) {
                        throw std::runtime_error("You have uncommitted changes! Please commit them first");
                    }
                }

                previous_content.files_map.erase(file_name);
            } else {
                throw std::runtime_error("You have uncommitted changes! Please commit them first");
            }
        }

        if (!previous_content.files_map.empty()) {
            throw std::runtime_error("You have uncommitted changes! Please commit them first");
        }
    }

    /**
     * Retrieves the current status of the repository, categorizing files into new, modified, staged, and deleted.
     *
     * @return A JitStatus object containing sets of new, modified, staged, and deleted files.
     */
    JitStatus ChangesManager::repo_status() {
        transform_file_names();

        auto file_map = get_files_map(files);

        JitStatus status;
        IndexFileParser parser(file_map, get_jit_root() + "/index");
        IndexFileContent previous_content = parser.read_index_file();
        std::set<FileInfo> new_files;
        std::set<FileInfo> modified_files;
        std::set<FileInfo> deleted_files;
        std::set<FileInfo> staged_files;

        for (auto const &file_info_pair : file_map) {
            auto file_name = file_info_pair.first;
            auto file_info = file_info_pair.second;

            if (previous_content.files_map.contains(file_name)) {
                auto old_file_info = previous_content.files_map.at(file_name);
                if (old_file_info.checksum != file_info.checksum) {
                    modified_files.insert(file_info);
                } else {
                    if (old_file_info.is_dirty) {
                        staged_files.insert(old_file_info);
                    }
                }

                previous_content.files_map.erase(file_name);
            } else {
                new_files.insert(file_info);
            }
        }


        // Add deleted files to the status
        for (auto const &file_info_pair : previous_content.files_map) {
            deleted_files.insert(file_info_pair.second);
        }

        // Fill the result map and return status
        status.new_files = new_files;
        status.modified_files = modified_files;
        status.staged_files = staged_files;
        status.deleted_files = deleted_files;

        return status;
    }

    /**
     * Prints the status of the repository, including staged, modified, and new files.
     *
     * @throws std::runtime_error if unable to open the HEAD file.
     */
    void ChangesManager::print_jit_status() {

        auto status = repo_status();

        auto new_files = status.new_files;
        auto modified_files = status.modified_files;
        auto deleted_files = status.deleted_files;
        auto staged_files = status.staged_files;

        std::ifstream head_file(get_jit_root() + "/HEAD");


        if (head_file) {
            std::string head;
            getline(head_file, head);

            if (head.starts_with("refs")) {
                std::cout << "On branch " << GREEN << fs::path(head).filename() << RESET
                          << std::endl;
            } else {
                std::cout << "HEAD detached at " << CYAN << head.substr(0, 7) << RESET << std::endl;
            }
        } else {
            throw std::runtime_error("Cannot open the head file");
        }

        if (new_files.empty() && modified_files.empty() && deleted_files.empty() && staged_files.empty()) {
            std::cout << GREEN << "nothing to commit, working tree clean" << RESET << std::endl;
            return;
        }

        // Print staged files
        if (!staged_files.empty()) {
            std::cout << "\nChanges to be committed:" << std::endl;
            for (const auto &file_info : staged_files) {
                std::string operation = file_info.is_dirty ? "modified" : file_info.is_new ? "new file" : "deleted";
                std::string operation_color = file_info.is_dirty ? YELLOW : file_info.is_new ? GREEN : RED;
                std::cout << operation_color << "\t" << operation << ": " << RESET << file_info.filename << std::endl;
            }
        }

        // Print modified and deleted files
        if (!modified_files.empty() || !deleted_files.empty()) {
            std::cout << "\nChanges not staged for commit:" << std::endl;
            for (const auto &file_info : modified_files) {
                std::cout << YELLOW << "\tmodified: " << RESET << file_info.filename << std::endl;
            }

            for (const auto &file_info : deleted_files) {
                std::cout << RED << "\tdeleted: " << RESET << file_info.filename << std::endl;
            }
        }

        // Print new files
        if (!new_files.empty()) {
            std::cout << "\nUntracked files:" << std::endl;
            for (const auto &file_info : new_files) {
                std::cout << RED << "\t" << file_info.filename << RESET << std::endl;
            }
        }
    }

    /**
     * Adds files to the repository index and stages them.
     *
     * @param file_names A set of file names to be added to the index and staged.
     */
    void ChangesManager::jit_add(const std::set<std::string> &file_names) {
        std::set<std::string> files_to_add;
        transform_file_names();

        // Process directories and files
        for (auto &file_name : file_names) {
            fs::path path = get_root_directory() + "/" + file_name;

            if (fs::is_directory(path)) {
                for (auto &directory : fs::recursive_directory_iterator(path)) {
                    auto cur_file_name = directory.path().lexically_normal().string();

                    if (!fs::is_directory(directory) && files.contains(cur_file_name))
                        files_to_add.insert(cur_file_name);
                }
            }
        }

        // Create and add file information to the index
        std::unordered_map<std::string, FileInfo> added_file_info = get_files_map(files_to_add);
        IndexFileParser parser(get_jit_root() + "/index");
        parser.create_index_file(added_file_info);

        // Stage added files
        std::set<FileInfo> staged_files;
        for (const auto &file_pair : added_file_info) {
            staged_files.insert(file_pair.second);
        }
        update_file_objects(staged_files);
    }

    /**
     * Updates the file objects (storing files as binary objects in the repository).
     *
     * @param file_infos A set of FileInfo objects to be saved as binary files.
     */
    void ChangesManager::update_file_objects(const std::set<FileInfo> &file_infos) {
        for (const auto &file_info : file_infos) {
            save_as_binary(get_jit_root() + "/objects", file_info.checksum, get_root_directory() + "/" + file_info.filename);
        }
    }

} // namespace manager
