//
// Created by thaiku on 30/11/24.
//

#ifndef CHANGESMANAGER_H
#define CHANGESMANAGER_H

#include <string>
#include <map>
#include <set>
#include "../DirectoryManagement/DirManager.h"
#include "IndexFileParser.h"
#include "data.h"

namespace manager {

    class ChangesManager : public DirManager {
    public:
        /**
         * Constructor that initializes the DirManager and sets the root directory.
         * @param root_directory The root directory of the repository.
         */
        explicit ChangesManager(const std::string &root_directory);

        /**
         * Generates a map of files with their metadata (e.g., checksum, modification date).
         *
         * @param files_to_add A set of file names to be processed and added to the map.
         * @return A map of filenames to FileInfo objects, each containing metadata of the corresponding file.
         * @throws std::runtime_error if any file in files_to_add cannot be found or read.
         */
        std::unordered_map<std::string, FileInfo> get_files_map(const std::set<std::string> &files_to_add);

        /**
         * Transforms file names by removing directory structure and leading slashes/dots.
         *
         * @return A set of transformed file names without directories.
         */
        std::set<std::string> transform_file_names();

        /**
         * Checks if the repository has uncommitted changes and throws an error if it does.
         *
         * @throws std::runtime_error if the repository has uncommitted changes.
         */
        void throw_error_if_repo_is_dirty();

        /**
         * Retrieves the current status of the repository, categorizing files into new, modified, staged, and deleted.
         *
         * @return A JitStatus object containing sets of new, modified, staged, and deleted files.
         */
        JitStatus repo_status();

        /**
         * Prints the status of the repository, including staged, modified, and new files.
         *
         * @throws std::runtime_error if unable to open the HEAD file.
         */
        void print_jit_status();

        /**
         * Adds files to the repository index and stages them.
         *
         * @param file_names A set of file names to be added to the index and staged.
         */
        void jit_add(const std::set<std::string> &file_names);

        /**
         * Updates the file objects (storing files as binary objects in the repository).
         *
         * @param file_infos A set of FileInfo objects to be saved as binary files.
         */
        void update_file_objects(const std::set<FileInfo> &file_infos);

    private:
        std::string jit_root;
        std::set<std::string> files;
    };

} // namespace manager

#endif // CHANGESMANAGER_H
