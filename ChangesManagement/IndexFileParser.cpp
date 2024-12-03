//
// Created by thaiku on 01/12/24.
//

#include "IndexFileParser.h"
#include "../JitUtility/jit_utility.h"
#include <fstream>
#include <chrono>
#include <iostream>
#include <utility>

namespace manager {


    void IndexFileParser::create_index_file(std::map<std::string, FileInfo> current_files) {
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

        content.files = files_info;
        content.metaData.is_dirty = a_file_changed;
        content.metaData.entries = current_files.size();
        content.files_map = files;
        this->index_file_content = content;
        this->files = current_files;

        write_index_file();
    }

    IndexMetaData IndexFileParser::metaDataCreator() {
        IndexMetaData metaData;
        metaData.entries = files.size();
        metaData.last_modified = std::chrono::high_resolution_clock::now();


        return metaData;
    }

    void IndexFileParser::prepare_commit_index_file() {
        IndexFileContent content = read_index_file();

        content.metaData.is_dirty = false;
        content.metaData.last_modified = std::chrono::system_clock::now();

        for (auto &file: content.files) {
            file.is_dirty = false;
            file.is_new = false;

            content.files_map.at(file.filename).is_dirty = false;
            content.files_map.at(file.filename).is_new = false;
        }

        this->index_file_content = content;
    }

    void IndexFileParser::write_index_file() {
        std::ofstream file(index_file_path);
        if (!file) {
            throw std::runtime_error("Could not open file for writing");
        }

        // Write metadata
        file << "[METADATA]\n";
        file << "entries = " << index_file_content.metaData.entries << "\n";
        file << "last_modified = " << time_point_to_string(index_file_content.metaData.last_modified) << "\n";
        file << "is_dirty = " << (index_file_content.metaData.is_dirty ? "true" : "false") << "\n";

        // Write file entries
        for (const auto &fileInfo: index_file_content.files) {
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

    IndexFileContent IndexFileParser::read_index_file() {
        std::ifstream file(index_file_path);

        if (!file) {
            throw std::runtime_error("Could not open file for reading");
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
                content.files.push_back(tempFileInfo);
                content.files_map[tempFileInfo.filename] = tempFileInfo;

                tempFileInfo = FileInfo{};
            }
        }

        if (!tempFileInfo.filename.empty()) {
            content.files.push_back(tempFileInfo);
            content.files_map[tempFileInfo.filename] = tempFileInfo;
        }

        file.close();
        return content;
    }


    IndexFileParser::IndexFileParser(const std::map<std::string, FileInfo> &files, std::string index_file_path) : files(
            files), index_file_path(std::move(index_file_path)) {

    }

    IndexFileParser::IndexFileParser(std::string index_file_path)
            : files(), index_file_path(std::move(index_file_path)) {}

}