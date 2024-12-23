//
// Created by thaiku on 02/12/24.
//

#include "jit_utility.h"

#include <filesystem>
#include <vector>
#include <zlib.h>
#include <fstream>
#include <iostream>
#include <openssl/sha.h>
#include <iomanip>
#include <regex>

/**
 * Converts a time_point to a string formatted as "YYYY-MM-DD HH:MM:SS".
 *
 * @param tp The time_point to be converted.
 * @return A string representation of the time_point.
 */
std::string time_point_to_string(const std::chrono::system_clock::time_point &tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::localtime(&time_t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

/**
 * Converts a string in "YYYY-MM-DD HH:MM:SS" format to a time_point.
 *
 * @param time_str The time string to be converted.
 * @return A time_point representation of the string.
 */
std::chrono::system_clock::time_point string_to_time_point(const std::string &time_str) {
    std::tm tm = {};
    std::istringstream iss(time_str);
    iss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

/**
 * Compresses the source file and writes the compressed data to the destination file.
 *
 * @param source The path to the source file.
 * @param destination The path to the destination file where the compressed data will be written.
 * @throws std::runtime_error If an error occurs during compression or file operations.
 */
void compress_and_copy(const std::string &source, const std::string &destination) {
    try {
        std::ifstream input(source, std::ios::binary);
        if (!input) {
            throw std::runtime_error("Cannot open source file for reading");
        }

        std::vector<char> buffer((std::istreambuf_iterator<char>(input)),
                                 std::istreambuf_iterator<char>());
        input.close();

        uLongf compressed_size = compressBound(buffer.size());
        std::vector<Bytef> compressed_data(compressed_size);

        int result = compress(compressed_data.data(), &compressed_size,
                              reinterpret_cast<const Bytef *>(buffer.data()), buffer.size());

        if (result != Z_OK) {
            throw std::runtime_error("Error compressing file data");
        }

        std::ofstream output(destination, std::ios::binary);
        if (!output) {
            throw std::runtime_error("Cannot open destination file for writing");
        }

        output.write(reinterpret_cast<const char *>(compressed_data.data()), static_cast<long>(compressed_size));
        output.close();

        // Optionally, set restrictive permissions
        fs::permissions(destination,
                        fs::perms::owner_read | fs::perms::group_read,
                        fs::perm_options::replace);
    } catch (const std::exception &e) {
        throw std::runtime_error(e.what());
    }
}

/**
 * Generates the SHA1 checksum of a file.
 *
 * @param file_path The path to the file whose checksum is to be generated.
 * @return The SHA1 checksum of the file in hexadecimal string format.
 */
std::string generateSHA1(const std::string &file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        std::cerr << "Error opening file: " << file_path << std::endl;
        return "";
    }

    // Initialize SHA-1 context
    SHA_CTX sha_ctx;
    SHA1_Init(&sha_ctx);

    const size_t buffer_size = 4096;
    char buffer[buffer_size];
    while (file.read(buffer, buffer_size)) {
        SHA1_Update(&sha_ctx, buffer, file.gcount());
    }
    SHA1_Update(&sha_ctx, buffer, file.gcount());

    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1_Final(hash, &sha_ctx);

    std::ostringstream oss;
    for (unsigned char i: hash) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int) i;
    }

    return oss.str();
}

/**
 * Saves a file in a binary format in a directory structure based on its checksum.
 *
 * @param destination The root directory where the file will be saved.
 * @param checksum The SHA1 checksum used for the directory structure.
 * @param file_name The path to the source file.
 */
void save_as_binary(const std::string &destination, const std::string &checksum, const std::string &file_name) {
    std::string checksum_prefix = checksum.substr(0, 2);
    std::string checksum_suffix = checksum.substr(2);

    fs::path sub_dir = fs::path(destination) / checksum_prefix;

    if (!fs::exists(sub_dir)) {
        fs::create_directories(sub_dir);
    }

    fs::path file_path = sub_dir / checksum_suffix;

    if (fs::exists(file_path)) {
        return;
    }

    std::ifstream source_file(file_name, std::ios::binary);

    if (!source_file) {
        std::cerr << "Error: Could not open source file: " << file_name << std::endl;
        return;
    }

    compress_and_copy(file_name, file_path);
}

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
             const std::string &cur_checksum, const std::string &message) {
    std::ofstream log_file(log_file_path, std::ios::app);

    if (log_file) {
        log_file << old_checksum << "\t" << cur_checksum << "\t"
                 << time_point_to_string(std::chrono::system_clock::now()) << "\t"
                 << message << std::endl;
        log_file.close();
    } else {
        throw std::runtime_error("Unable to open the log file");
    }
}

/**
 * Prints the commit log from a file to the console with colored output.
 *
 * @param file_path The path to the log file.
 */
void print_commit_log(const std::string &file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "Could not open the file!" << std::endl;
        return;
    }

    std::string line;
    std::regex commit_regex(
            R"((^[0-9a-f]{40})\s([0-9a-f]{40})\s(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})\s+(?:commit|merge):\s(.+))");
    std::smatch match;

    while (std::getline(file, line)) {
        if (std::regex_search(line, match, commit_regex)) {
            std::string commit_hash = match[2].str(); // Use the second SHA1 hash
            std::string commit_date = match[3].str();
            std::string commit_message = match[4].str();

            // Adding colors to output
            std::cout << "\033[1;32mcommit\033[0m " << commit_hash << std::endl;  // Green for 'commit'
            std::cout << "\033[1;34mAuthor:\033[0m Unknown" << std::endl;  // Blue for 'Author'
            std::cout << "\033[1;36mDate:\033[0m   " << commit_date << std::endl;  // Cyan for 'Date'
            std::cout << std::endl;
            std::cout << "    \033[1;33m" << commit_message << "\033[0m" << std::endl;  // Yellow for commit message
            std::cout << std::endl;
        }
    }

    file.close();
}

/**
 * Decompresses the source file and writes the decompressed data to the destination file.
 *
 * @param source The path to the compressed source file.
 * @param destination The path to the destination file where the decompressed data will be written.
 * @throws std::runtime_error If an error occurs during decompression or file operations.
 */
void decompress_and_copy(const std::string &source, const std::string &destination) {
    try {
        std::ifstream input(source, std::ios::binary);
        if (!input) {
            throw std::runtime_error("Cannot open source " + source + " for reading");
        }

        std::vector<char> compressed_data((std::istreambuf_iterator<char>(input)),
                                          std::istreambuf_iterator<char>());
        input.close();

        uLongf decompressed_size = compressed_data.size() * 4; // Size estimate (it may need adjustment)
        std::vector<Bytef> decompressed_data(decompressed_size);

        int result = uncompress(decompressed_data.data(), &decompressed_size,
                                reinterpret_cast<const Bytef *>(compressed_data.data()), compressed_data.size());

        if (result != Z_OK) {
            throw std::runtime_error("Error decompressing file data");
        }

        fs::path destination_dir = fs::path(destination).parent_path();

        // Create any missing directories in the destination path
        if (!fs::exists(destination_dir)) {
            fs::create_directories(destination_dir);
        }


        std::ofstream output(destination, std::ios::binary);
        if (!output) {
            throw std::runtime_error("Cannot open destination file for writing");
        }

        output.write(reinterpret_cast<const char *>(decompressed_data.data()), static_cast<long>(decompressed_size));
        output.close();

        fs::permissions(destination,
                        fs::perms::owner_read | fs::perms::group_read | fs::perms::others_read |
                        fs::perms::owner_write | fs::perms::group_write,
                        fs::perm_options::replace);
    } catch (const std::exception &e) {
        throw std::runtime_error(e.what());
    }
}

/**
 * Generates the file path for a file based on its checksum.
 *
 * @param checksum The SHA1 checksum of the file.
 * @return The file path based on the checksum.
 */
fs::path generate_file_path(const std::string &checksum) {
    std::string checksum_prefix = checksum.substr(0, 2);
    std::string checksum_suffix = checksum.substr(2);

    return fs::path(checksum_prefix) / fs::path(checksum_suffix);
}

/**
 * Computes the Longest Common Subsequence (LCS) table between two files represented as vectors of strings.
 *
 * @param file1 The first file represented as a vector of strings.
 * @param file2 The second file represented as a vector of strings.
 * @return A 2D vector representing the LCS table.
 */
std::vector<std::vector<int>>
compute_lcs_table(const std::vector<std::string> &file1, const std::vector<std::string> &file2) {
    size_t n = file1.size(), m = file2.size();
    std::vector<std::vector<int>> lcs_table(n + 1, std::vector<int>(m + 1, 0));

    for (size_t i = 1; i <= n; ++i) {
        for (size_t j = 1; j <= m; ++j) {
            if (file1[i - 1] == file2[j - 1]) {
                lcs_table[i][j] = lcs_table[i - 1][j - 1] + 1;
            } else {
                lcs_table[i][j] = std::max(lcs_table[i - 1][j], lcs_table[i][j - 1]);
            }
        }
    }
    return lcs_table;
}

/**
 * Generates a diff between two files represented as vectors of strings based on the LCS table.
 *
 * @param file1 The first file represented as a vector of strings.
 * @param file2 The second file represented as a vector of strings.
 * @param lcs_table The LCS table computed between the two files.
 * @return A vector of strings representing the diff between the files.
 */
std::vector<std::string> generate_diff(const std::vector<std::string> &file1, const std::vector<std::string> &file2,
                                       const std::vector<std::vector<int>> &lcs_table) {
    size_t i = file1.size(), j = file2.size();
    std::vector<std::string> diff;

    while (i > 0 || j > 0) {
        if (i > 0 && j > 0 && file1[i - 1] == file2[j - 1]) {
            diff.push_back("  " + file1[i - 1]);
            --i;
            --j;
        } else if (j > 0 && (i == 0 || lcs_table[i][j - 1] >= lcs_table[i - 1][j])) {
            diff.push_back("+ " + file2[j - 1]);
            --j;
        } else if (j == 0 || lcs_table[i][j - 1] < lcs_table[i - 1][j]) {
            diff.push_back("- " + file1[i - 1]);
            --i;
        }
    }

    std::reverse(diff.begin(), diff.end());

    return diff;
}

/**
 * Reads a binary file, decompresses its content, and returns the content as a vector of strings (lines).
 *
 * @param source The path to the compressed binary source file.
 * @return A vector of strings representing the decompressed content, line by line.
 */
std::vector<std::string> read_binary_as_text(const std::string &source) {
    try {
        // Open the compressed source file
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

        // Split the decompressed text into lines
        std::vector<std::string> lines;
        std::istringstream text_stream(decompressed_text);
        std::string line;
        while (std::getline(text_stream, line)) {
            lines.push_back(line);
        }

        return lines;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return {};
    }
}

/**
 * Computes and generates a diff between two files represented as vectors of strings.
 *
 * @param file1 The first file represented as a vector of strings.
 * @param file2 The second file represented as a vector of strings.
 * @return A vector of strings representing the diff between the files.
 */
std::vector<std::string> compute_diff(const std::vector<std::string> &file1, const std::vector<std::string> &file2) {
    // Calculate LCS table
    std::vector<std::vector<int>> lcs_table = compute_lcs_table(file1, file2);

    // Generate and return the diff
    return generate_diff(file1, file2, lcs_table);
}
