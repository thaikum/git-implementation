//
// Created by thaiku on 01/12/24.
//

#ifndef JIT_INDEXFILEPARSER_H
#define JIT_INDEXFILEPARSER_H

#include "data.h"
#include <chrono>
#include <map>

namespace manager {
    /**
     * Class responsible for parsing, reading, writing, and creating index files.
     */
    class IndexFileParser {
    public:
        /**
         * @brief Constructs an `IndexFileParser` object with a given set of files and an index file path.
         *
         * This constructor initializes the parser with the provided files and
         * index file path.
         *
         * @param files A map of file names to their corresponding file info.
         * @param index_file_path The path to the index file.
         */
        IndexFileParser(const std::unordered_map<std::string, FileInfo> &files, std::string index_file_path);


        /**
         * Constructor that initializes the parser with the index file path only.
         *
         * @param index_file_path The path to the index file.
         */
        explicit IndexFileParser(std::string index_file_path);

        /**
         * Reads the index file and returns the content as an `IndexFileContent` object.
         *
         * @return The parsed content of the index file.
         * @throws std::runtime_error if the index file cannot be read.
         */
        IndexFileContent read_index_file();

        /**
         * Writes the current `index_file_content` to the index file.
         *
         * @throws std::runtime_error if the index file cannot be written.
         */
        void write_index_file();

        void write_index_file(const IndexFileContent &content);


        /**
         * Creates a new index file from the provided map of files.
         *
         * @param current_files A map of file names to `FileInfo` objects.
         * @throws std::runtime_error if the index file cannot be created.
         */
        void create_index_file(std::unordered_map<std::string, FileInfo> current_files);

        /**
         * Prepares the index file for a commit by performing necessary setup.
         *
         * @throws std::runtime_error if the index file cannot be prepared.
         */
        void prepare_commit_index_file();

        static IndexFileContent read_binary_index_file(const std::string &source);

    private:
        /**
         * A unordered_map of file names to their corresponding `FileInfo` objects.
         */
        std::unordered_map<std::string, FileInfo> files;

        /**
         * Creates metadata for the index file.
         *
         * @return The generated `IndexMetaData` object.
         */
        [[maybe_unused]] IndexMetaData metaDataCreator();

        /**
         * The path to the index file.
         */
        const std::string index_file_path;

        /**
         * The content of the index file, parsed into an `IndexFileContent` object.
         */
        IndexFileContent index_file_content;

    };
}

#endif //JIT_INDEXFILEPARSER_H
