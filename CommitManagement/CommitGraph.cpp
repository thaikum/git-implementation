//
// Created by thaiku on 08/12/24.
//

#include "CommitGraph.h"
#include "../JitUtility/jit_utility.h"
#include <iostream>
#include <unordered_set>
#include <fstream>
#include <sstream>
#include <utility>
#include <zlib.h>
#include <filesystem>

namespace fs = std::filesystem;

namespace manager {
    void CommitGraph::add_commit(const Commit &commit) {
        commits.insert({commit.checksum, commit});
    }

    void CommitGraph::add_commit(Commit commit, const std::vector<std::string> &parents) {
        std::cout << "A commit was added " << std::endl;
        std::vector<std::string> pointers_to_parents;
        pointers_to_parents.reserve(parents.size());

        for (const auto &parent: parents) {
            if (commits.contains(parent))
                pointers_to_parents.push_back(parent);
        }
        commit.parents = pointers_to_parents;
        add_commit(commit);
    }

    std::shared_ptr<Commit> CommitGraph::get_commit(const std::string &checksum) {
        auto commit = commits.find(checksum);
        if (commit == commits.end()) {
            return nullptr;
        } else {
            return std::make_unique<Commit>(commits.at(checksum));
        }
    }

    using CommitMap = std::unordered_map<std::string, Commit>;

    std::shared_ptr<Commit>
    CommitGraph::get_intersection_commit(const std::string &checksum1, const std::string &checksum2) {
        // Find the commits corresponding to the checksums
        auto commit1 = commits.find(checksum1);
        auto commit2 = commits.find(checksum2);

        if (commit1 == commits.end() || commit2 == commits.end()) {
            std::cerr << "One or both commits not found" << std::endl;
            return nullptr;
        }

        std::unordered_set<std::string> ancestors;

        // Traverse parents of the first commit and store all ancestors
        std::vector<Commit> stack1 = {commit1->second};
        while (!stack1.empty()) {
            auto current = stack1.back();
            stack1.pop_back();
            if (ancestors.insert(current.checksum).second) {  // Insert and check if it was not already present
                for (const auto &parent: current.parents) {
                    stack1.push_back(commits.at(parent));
                }
            }
        }

        std::vector<Commit> stack2 = {commit2->second};
        std::vector<Commit> intersections;

        while (!stack2.empty()) {
            auto current = stack2.back();
            stack2.pop_back();
            if (ancestors.find(current.checksum) != ancestors.end()) {
                intersections.push_back(current);  // Intersection found
            }

            for (const auto &parent: current.parents) {
                stack2.push_back(commits.at(parent));
            }
        }

        if (intersections.empty()) {
            return nullptr;  // No intersection found
        } else {
            std::sort(intersections.begin(), intersections.end(),
                      [](const Commit &commit1, const Commit &commit2) {
                          return commit2.timestamp < commit1.timestamp;
                      });
            Commit commit = intersections.front();
            return std::make_unique<Commit>(commit);
        }
    }

    void pretty_print(const Commit &commit, const std::string &addition) {
        std::string add = addition.empty() ? "" : (" (" + addition + ")");
        std::cout << GREEN << commit.checksum << YELLOW << add << std::endl;  // Green for 'commit'
        std::cout << BLUE << "Author: " << RESET << "Unknown" << std::endl;  // Blue for 'Author'
        std::cout << CYAN << "Date:  " << RESET << time_point_to_string(commit.timestamp)
                  << std::endl;  // Cyan for 'Date'
        std::cout << std::endl;
        std::cout << YELLOW << "\t" << commit.message << RESET << std::endl;  // Yellow for commit message
        std::cout << std::endl;
    }

    void CommitGraph::print_commit_history(std::string checksum) const {

        while (commits.contains(checksum)) {

            auto commit = commits.at(checksum);

            pretty_print(commit, "");

            for (const auto &p_checksum: commit.parents) {
                auto parent = commits.at(p_checksum);
                if (parent.branch_name == commit.branch_name) {
                    checksum = parent.checksum;
                } else {
                    pretty_print(parent, parent.branch_name);
                }
            }

            //when no parent was found
            if (checksum == commit.checksum)
                break;
        }
    }

    void CommitGraph::save_commits(const std::string &file_path) {
        fs::path destination_dir = fs::path(file_path).parent_path();
        std::cout << "The size id: " << commits.size() << std::endl;

        // Create any missing directories in the destination path
        if (!fs::exists(destination_dir)) {
            fs::create_directories(destination_dir);
        }

        std::ostringstream oss;

        size_t map_size = commits.size();
        oss.write(reinterpret_cast<const char *>(&map_size), sizeof(map_size));

        for (const auto &[_, commit]: commits) {
            size_t checksum_size = commit.checksum.size();
            oss.write(reinterpret_cast<const char *>(&checksum_size), sizeof(checksum_size));
            oss.write(commit.checksum.data(), checksum_size);

            size_t message_size = commit.message.size();
            oss.write(reinterpret_cast<const char *>(&message_size), sizeof(message_size));
            oss.write(commit.message.data(), message_size);

            size_t branch_size = commit.branch_name.size();
            oss.write(reinterpret_cast<const char *>(&branch_size), sizeof(branch_size));
            oss.write(commit.branch_name.data(), branch_size);

            size_t author_size = commit.author.size();
            oss.write(reinterpret_cast<const char *>(&author_size), sizeof(author_size));
            oss.write(commit.author.data(), author_size);

            auto time = commit.timestamp.time_since_epoch().count();
            oss.write(reinterpret_cast<const char *>(&time), sizeof(time));

            size_t parents_size = commit.parents.size();
            oss.write(reinterpret_cast<const char *>(&parents_size), sizeof(parents_size));
            for (const auto &parent: commit.parents) {
                size_t parent_size = parent.size();
                oss.write(reinterpret_cast<const char *>(&parent_size), sizeof(parent_size));
                oss.write(parent.data(), parent_size);
            }
        }

        std::string serialized_data = oss.str();

        // Compress the data
        uLong compressed_size = compressBound(serialized_data.size());
        std::vector<char> compressed_data(compressed_size);
        int result = compress(reinterpret_cast<Bytef *>(compressed_data.data()), &compressed_size,
                              reinterpret_cast<const Bytef *>(serialized_data.data()), serialized_data.size());

        if (result != Z_OK) {
            throw std::runtime_error("Compression failed");
        }

        // Write compressed data to the file
        std::ofstream out(file_path, std::ios::binary);
        if (!out) {
            throw std::runtime_error("Failed to open file for writing: " + file_path);
        }

        out.write(reinterpret_cast<const char *>(&compressed_size), sizeof(compressed_size));
        out.write(compressed_data.data(), compressed_size);

        out.close();
    }

    void CommitGraph::load_commits(const std::string &file_path) {
        commits.clear();
        std::ifstream in(file_path, std::ios::binary);
        if (!in) {
            throw std::runtime_error("Failed to open file for reading: " + file_path);
        }

        // Read the compressed size
        uLong compressed_size;
        in.read(reinterpret_cast<char *>(&compressed_size), sizeof(compressed_size));

        // Read the compressed data
        std::vector<char> compressed_data(compressed_size);
        in.read(compressed_data.data(), compressed_size);

        // Decompress the data
        std::vector<char> decompressed_data(10 * compressed_size);  // Assuming a reasonable decompression size
        uLong decompressed_size = decompressed_data.size();

        int result = uncompress(reinterpret_cast<Bytef *>(decompressed_data.data()), &decompressed_size,
                                reinterpret_cast<const Bytef *>(compressed_data.data()), compressed_size);

        if (result == Z_BUF_ERROR) {
            throw std::runtime_error("Buffer size is insufficient for decompression");
        } else if (result != Z_OK) {
            throw std::runtime_error("Decompression failed");
        }

        // Convert decompressed data to an input stream
        std::istringstream iss(std::string(decompressed_data.data(), decompressed_size));

        size_t map_size;
        iss.read(reinterpret_cast<char *>(&map_size), sizeof(map_size));

        for (size_t i = 0; i < map_size; ++i) {
            Commit commit;

            size_t checksum_size;
            iss.read(reinterpret_cast<char *>(&checksum_size), sizeof(checksum_size));
            commit.checksum.resize(checksum_size);
            iss.read(&commit.checksum[0], checksum_size);

            size_t message_size;
            iss.read(reinterpret_cast<char *>(&message_size), sizeof(message_size));
            commit.message.resize(message_size);
            iss.read(&commit.message[0], message_size);

            size_t branch_size;
            iss.read(reinterpret_cast<char *>(&branch_size), sizeof(branch_size));
            commit.branch_name.resize(branch_size);
            iss.read(&commit.branch_name[0], branch_size);

            size_t author_size;
            iss.read(reinterpret_cast<char *>(&author_size), sizeof(author_size));
            commit.author.resize(author_size);
            iss.read(&commit.author[0], author_size);

            long long time;
            iss.read(reinterpret_cast<char *>(&time), sizeof(time));
            commit.timestamp = std::chrono::system_clock::time_point(std::chrono::system_clock::duration(time));

            size_t parents_size;
            iss.read(reinterpret_cast<char *>(&parents_size), sizeof(parents_size));
            commit.parents.resize(parents_size);
            for (size_t j = 0; j < parents_size; ++j) {
                size_t parent_size;
                iss.read(reinterpret_cast<char *>(&parent_size), sizeof(parent_size));
                std::string parent(parent_size, '\0');
                iss.read(&parent[0], parent_size);
                commit.parents[j] = parent;
            }

            commits[commit.checksum] = commit;
        }

        in.close();
    }

    CommitGraph::CommitGraph(std::string commit_file_path) : commit_file_path(std::move(commit_file_path)) {
        if (fs::exists(this->commit_file_path)) {
            load_commits(this->commit_file_path);
        }
    }

}
