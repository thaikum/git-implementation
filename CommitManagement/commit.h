//
// Created by thaiku on 08/12/24.
//

#ifndef JIT_COMMIT_H
#define JIT_COMMIT_H
#include <string>
#include <chrono>
#include <vector>

struct Commit{
    std::string checksum;
    std::string message;
    std::string branch_name;
    std::string author;
    std::chrono::system_clock::time_point timestamp;
    std::vector<std::string> parents;
};
#endif //JIT_COMMIT_H
