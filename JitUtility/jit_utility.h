//
// Created by thaiku on 02/12/24.
//

#ifndef JIT_JIT_UTILITY_H
#define JIT_JIT_UTILITY_H

#include <string>
#include <vector>
#include <filesystem>

#define RESET "\033[0m"
#define GREEN "\033[1;32m"
#define RED "\033[1;31m"
#define YELLOW "\033[1;33m"
#define CYAN "\033[1;36m"
#define BLUE "\033[1;32m"

namespace fs = std::filesystem;

/**
* Converts a time_point to a string formatted as "YYYY-MM-DD HH:MM:SS".
*
* @param tp The time_point to be converted.
* @return A string representation of the time_point.
*/
std::string time_point_to_string(const std::chrono::system_clock::time_point &tp);

/**
 * Converts a string in "YYYY-MM-DD HH:MM:SS" format to a time_point.
 *
 * @param time_str The time string to be converted.
 * @return A time_point representation of the string.
 */
std::chrono::system_clock::time_point string_to_time_point(const std::string &time_str);

/**
 * Compresses the source file and writes the compressed data to the destination file.
 *
 * @param source The path to the source file.
 * @param destination The path to the destination file where the compressed data will be written.
 * @throws std::runtime_error If an error occurs during compression or file operations.
 */
void compress_and_copy(const std::string &source, const std::string &destination);

/**
 * Generates the SHA1 checksum of a file.
 *
 * @param file_path The path to the file whose checksum is to be generated.
 * @return The SHA1 checksum of the file in hexadecimal string format.
 */
std::string generateSHA1(const std::string &file_path);

/**
 * Saves a file in a binary format in a directory structure based on its checksum.
 *
 * @param destination The root directory where the file will be saved.
 * @param checksum The SHA1 checksum used for the directory structure.
 * @param file_name The path to the source file.
 */
void save_as_binary(const std::string &destination, const std::string &checksum, const std::string &file_name);

/**
 * Logs the details of a checksum change event to a log file.
 *
 * @param log_file_path The path to the log file.
 * @param old_checksum The old checksum value.
 * @param cur_checksum The new checksum value.
 * @param message A message describing the event.
 * @throws std::runtime_error If there is an issue opening or writing to the log file.
 */
void jit_log(const std::string &log_file_path, const std::string &old_checksum,
             const std::string &cur_checksum, const std::string &message);

/**
 * Prints the commit log from a file to the console with colored output.
 *
 * @param file_path The path to the log file.
 */
void print_commit_log(const std::string &file_path);

/**
 * Decompresses the source file and writes the decompressed data to the destination file.
 *
 * @param source The path to the compressed source file.
 * @param destination The path to the destination file where the decompressed data will be written.
 * @throws std::runtime_error If an error occurs during decompression or file operations.
 */
void decompress_and_copy(const std::string &source, const std::string &destination);

/**
 * Generates the file path for a file based on its checksum.
 *
 * @param checksum The SHA1 checksum of the file.
 * @return The file path based on the checksum.
 */
std::filesystem::path generate_file_path(const std::string &checksum);

/**
 * Computes the Longest Common Subsequence (LCS) table between two files represented as vectors of strings.
 *
 * @param file1 The first file represented as a vector of strings.
 * @param file2 The second file represented as a vector of strings.
 * @return A 2D vector representing the LCS table.
 */
std::vector<std::vector<int>>
compute_lcs_table(const std::vector<std::string> &file1, const std::vector<std::string> &file2);

/**
 * Generates a diff between two files represented as vectors of strings based on the LCS table.
 *
 * @param file1 The first file represented as a vector of strings.
 * @param file2 The second file represented as a vector of strings.
 * @param lcs_table The LCS table computed between the two files.
 * @return A vector of strings representing the diff between the files.
 */
std::vector<std::string> generate_diff(const std::vector<std::string> &file1, const std::vector<std::string> &file2,
                                       const std::vector<std::vector<int>> &lcs_table);

/**
 * Reads a binary file, decompresses its content, and returns the content as a vector of strings (lines).
 *
 * @param source The path to the compressed binary source file.
 * @return A vector of strings representing the decompressed content, line by line.
 */
std::vector<std::string> read_binary_as_text(const std::string &source);

/**
 * Computes and generates a diff between two files represented as vectors of strings.
 *
 * @param file1 The first file represented as a vector of strings.
 * @param file2 The second file represented as a vector of strings.
 * @return A vector of strings representing the diff between the files.
 */
std::vector<std::string> compute_diff(const std::vector<std::string> &file1, const std::vector<std::string> &file2);

#endif //JIT_JIT_UTILITY_H
