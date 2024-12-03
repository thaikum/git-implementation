//
// Created by thaiku on 01/12/24.
//

#ifndef JIT_INDEXFILEPARSER_H
#define JIT_INDEXFILEPARSER_H

#include "data.h"
#include <chrono>
#include <map>

namespace manager {
    class IndexFileParser {
    public:
        IndexFileParser(const std::map<std::string, FileInfo> &files, std::string index_file_path);

        explicit IndexFileParser(std::string index_file_path);

        IndexFileContent read_index_file();

        void write_index_file();

        void create_index_file(std::map<std::string, FileInfo> current_files);

        void prepare_commit_index_file();


    private:
        std::map<std::string, FileInfo> files;

        IndexMetaData metaDataCreator();

        const std::string index_file_path;

        IndexFileContent index_file_content;

    };
}


#endif //JIT_INDEXFILEPARSER_H
