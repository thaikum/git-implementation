//
// Created by thaiku on 01/12/24.
//

#include "IndexFileParser.h"
#include "../JitUtility/jit_utility.h"
#include <fstream>
#include <chrono>
#include <iostream>
#include <utility>
#include <zlib.h>

namespace manager {

    /**
     * @brief Creates or updates the index file with the current files' information.
     *
     * This function updates the index file by checking if files have changed,
     * marking them as dirty or new, and then writing the updated information back
     * to the index file.
     *
     * @param current_files A map of current file names to their corresponding file info.
     * @throws std::runtime_error If there is an issue writing to the index file.
     */
    void IndexFileParser::create_index_file(std::unordered_map<std::string, FileInfo> current_files) {
        IndexFileContent content = read_index_file();
        files = content.files_map;
        bool a_file_changed = false;
        std::vector<FileInfo> files_info;

        for (auto &file: current_files) {
            if (files.contains(file.first)) {
                auto old_info = files.at(file.first);
                if (file.second.checksum != old_info.checksum) {
                    file.second.is_dirty = true;
                    file.second.is_new = false;
                    a_file_changed = true;
                    files.insert(file);
                }
            } else {
                file.second.is_dirty = true;
                file.second.is_new = true;
                files.insert(file);
                a_file_changed = true;
            }
            files_info.push_back(file.second);
        }

        content.metaData.is_dirty = a_file_changed;
        content.metaData.entries = current_files.size();
        content.files_map = files;
        this->index_file_content = content;
        this->files = current_files;

        write_index_file();
    }

    /**
     * @brief Creates the metadata for the index file.
     *
     * This function generates metadata based on the current file count and
     * the current time, which is used when writing the index file.
     *
     * @return IndexMetaData A populated metadata object with file count and timestamp.
     */
    [[maybe_unused]] IndexMetaData IndexFileParser::metaDataCreator() {
        IndexMetaData metaData;
        metaData.entries = files.size();
        metaData.last_modified = std::chrono::high_resolution_clock::now();
        return metaData;
    }

    /**
     * @brief Prepares the index file for a commit.
     *
     * This function resets the `dirty` and `new` flags in the index file entries
     * and updates the metadata to reflect that the index is clean and ready for commit.
     *
     * @throws std::runtime_error If there is an issue reading or writing to the index file.
     */
    void IndexFileParser::prepare_commit_index_file() {
        IndexFileContent content = read_index_file();

        content.metaData.is_dirty = false;
        content.metaData.last_modified = std::chrono::system_clock::now();

        for (auto &[_, file]: content.files_map) {
            content.files_map.at(file.filename).is_dirty = false;
            content.files_map.at(file.filename).is_new = false;
        }

        this->index_file_content = content;
    }

    void IndexFileParser::write_index_file(){
        write_index_file(index_file_content);
    }

    /**
     * @brief Writes the current state of the index file to disk.
     *
     * This function writes both the metadata and the file entries to the
     * specified index file path.
     *
     * @throws std::runtime_error If there is an issue writing to the index file.
     */
    void IndexFileParser::write_index_file(const IndexFileContent& content) {
        std::ofstream file(index_file_path);
        if (!file) {
            throw std::runtime_error("Could not open file for writing");
        }

        // Write metadata
        file << "[METADATA]\n";
        file << "entries = " << content.metaData.entries << "\n";
        file << "last_modified = " << time_point_to_string(content.metaData.last_modified) << "\n";
        file << "is_dirty = " << (content.metaData.is_dirty ? "true" : "false") << "\n";

        // Write file entries
        for (const auto &[_, fileInfo]: content.files_map) {
            file << "\n[ENTRY]\n";
            file << "filename = " << fileInfo.filename << "\n";
            file << "checksum = " << fileInfo.checksum << "\n";
            file << "addition_date = " << time_point_to_string(fileInfo.addition_date) << "\n";
            file << "last_modified = " << time_point_to_string(fileInfo.last_modified) << "\n";
            file << "is_dirty = " << (fileInfo.is_dirty ? "true" : "false") << "\n";
            file << "is_new = " << (fileInfo.is_new ? "true" : "false") << "\n";
            file << "\n";
        }
        file.close();
    }

    /**
     * @brief Reads the index file and loads its contents.
     *
     * This function reads the index file, parses it, and populates an
     * `IndexFileContent` structure with the file metadata and file information.
     *
     * @return IndexFileContent The parsed contents of the index file.
     * @throws std::runtime_error If the index file cannot be opened or read.
     */
    IndexFileContent IndexFileParser::read_index_file() {
        std::ifstream file(index_file_path);

        if (!file) {
            throw std::runtime_error("Could not open file: " + index_file_path);
        }

        IndexFileContent content;
        std::string line;
        FileInfo tempFileInfo;
        bool readingFiles = false;

        while (std::getline(file, line)) {
            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t") + 1);

            if (line == "[METADATA]") {
                readingFiles = false;
            } else if (line == "[ENTRY]") {
                readingFiles = true;
            } else if (line.find('=') != std::string::npos) {
                auto delimiterPos = line.find('=');
                std::string key = line.substr(0, delimiterPos);
                std::string value = line.substr(delimiterPos + 1);
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);

                if (!readingFiles) {
                    if (key == "entries") content.metaData.entries = std::stoi(value);
                    else if (key == "last_modified") content.metaData.last_modified = string_to_time_point(value);
                    else if (key == "is_dirty") content.metaData.is_dirty = (value == "true");
                } else {
                    if (key == "filename") tempFileInfo.filename = value;
                    else if (key == "checksum") tempFileInfo.checksum = value;
                    else if (key == "addition_date") tempFileInfo.addition_date = string_to_time_point(value);
                    else if (key == "last_modified") tempFileInfo.last_modified = string_to_time_point(value);
                    else if (key == "is_dirty") tempFileInfo.is_dirty = (value == "true");
                    else if (key == "is_new") tempFileInfo.is_new = (value == "true");
                }
            } else if (line.empty() && readingFiles) {
                readingFiles = false;
                content.files_map[tempFileInfo.filename] = tempFileInfo;

                tempFileInfo = FileInfo{};
            }
        }

        if (!tempFileInfo.filename.empty()) {
            content.files_map[tempFileInfo.filename] = tempFileInfo;
        }

        file.close();
        return content;
    }

    IndexFileContent IndexFileParser::read_binary_index_file(const std::string &source) {
        try {
            // Open the compressed binary source file
            std::ifstream input(source, std::ios::binary);
            if (!input) {
                throw std::runtime_error("Cannot open source " + source + " for reading");
            }

            // Read the compressed data into a buffer
            std::vector<char> compressed_data((std::istreambuf_iterator<char>(input)),
                                              std::istreambuf_iterator<char>());
            input.close();

            // Prepare the decompression buffer
            uLongf decompressed_size = compressed_data.size() * 4; // Initial size estimate
            std::vector<Bytef> decompressed_data(decompressed_size);

            // Decompress the data
            int result = uncompress(decompressed_data.data(), &decompressed_size,
                                    reinterpret_cast<const Bytef *>(compressed_data.data()), compressed_data.size());

            if (result == Z_BUF_ERROR) {
                throw std::runtime_error("Buffer size is too small for decompression");
            } else if (result != Z_OK) {
                throw std::runtime_error("Error decompressing file data");
            }

            // Resize decompressed data to actual size
            decompressed_data.resize(decompressed_size);

            // Convert decompressed data to string
            std::string decompressed_text(decompressed_data.begin(), decompressed_data.end());

            // Now, parse the decompressed text into IndexFileContent
            IndexFileContent content;
            std::string line;
            FileInfo tempFileInfo;
            bool readingFiles = false;
            std::istringstream text_stream(decompressed_text);

            while (std::getline(text_stream, line)) {
                line.erase(0, line.find_first_not_of(" \t"));
                line.erase(line.find_last_not_of(" \t") + 1);

                if (line == "[METADATA]") {
                    readingFiles = false;
                } else if (line == "[ENTRY]") {
                    readingFiles = true;
                } else if (line.find('=') != std::string::npos) {
                    auto delimiterPos = line.find('=');
                    std::string key = line.substr(0, delimiterPos);
                    std::string value = line.substr(delimiterPos + 1);
                    key.erase(0, key.find_first_not_of(" \t"));
                    key.erase(key.find_last_not_of(" \t") + 1);
                    value.erase(0, value.find_first_not_of(" \t"));
                    value.erase(value.find_last_not_of(" \t") + 1);

                    if (!readingFiles) {
                        if (key == "entries") content.metaData.entries = std::stoi(value);
                        else if (key == "last_modified") content.metaData.last_modified = string_to_time_point(value);
                        else if (key == "is_dirty") content.metaData.is_dirty = (value == "true");
                    } else {
                        if (key == "filename") tempFileInfo.filename = value;
                        else if (key == "checksum") tempFileInfo.checksum = value;
                        else if (key == "addition_date") tempFileInfo.addition_date = string_to_time_point(value);
                        else if (key == "last_modified") tempFileInfo.last_modified = string_to_time_point(value);
                        else if (key == "is_dirty") tempFileInfo.is_dirty = (value == "true");
                        else if (key == "is_new") tempFileInfo.is_new = (value == "true");
                    }
                } else if (line.empty() && readingFiles) {
                    readingFiles = false;
                    content.files_map[tempFileInfo.filename] = tempFileInfo;

                    tempFileInfo = FileInfo{};
                }
            }

            if (!tempFileInfo.filename.empty()) {
                content.files_map[tempFileInfo.filename] = tempFileInfo;
            }

            return content;
        } catch (const std::exception &e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return {}; // Return empty content if there is an error
        }
    }


    /**
     * @brief Constructs an `IndexFileParser` object with a given set of files and an index file path.
     *
     * This constructor initializes the parser with the provided files and
     * index file path.
     *
     * @param files A map of file names to their corresponding file info.
     * @param index_file_path The path to the index file.
     */
    IndexFileParser::IndexFileParser(const std::unordered_map<std::string, FileInfo> &files, std::string index_file_path) : files(
            files), index_file_path(std::move(index_file_path)) {}

    /**
     * @brief Constructs an `IndexFileParser` object with an index file path.
     *
     * This constructor initializes the parser with an empty set of files and
     * the provided index file path.
     *
     * @param index_file_path The path to the index file.
     */
    IndexFileParser::IndexFileParser(std::string index_file_path)
            : files(), index_file_path(std::move(index_file_path)) {}

}
