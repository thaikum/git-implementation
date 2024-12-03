//
// Created by thaiku on 02/12/24.
//

#ifndef JIT_JIT_UTILITY_H
#define JIT_JIT_UTILITY_H

#include <string>
#include <filesystem>

#define RESET "\033[0m"
#define GREEN "\033[1;32m"
#define RED "\033[1;31m"
#define YELLOW "\033[1;33m"
#define BLUE "\033[1;34m"
#define CYAN "\033[1;36m"

namespace fs = std::filesystem;

void compress_and_copy(const std::string &source, const std::string &destination);
std::string generateSHA1(const std::string &file_path);
void save_as_binary(const std::string& destination, const std::string& checksum, const std::string& file_name);
std::string time_point_to_string(const std::chrono::system_clock::time_point& tp);
std::chrono::system_clock::time_point string_to_time_point(const std::string& time_str);
void jit_log(const std::string &log_file_path, const std::string &old_checksum,
             const std::string &cur_checksum, const std::string& message);
void print_commit_log(const std::string& file_path);
void decompress_and_copy(const std::string &source, const std::string &destination);
fs::path generate_file_path(const std::string& checksum);

#endif //JIT_JIT_UTILITY_H
