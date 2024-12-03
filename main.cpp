#include <iostream>
#include <vector>
#include "DirectoryManagent/DirManager.h"
#include "ChangesManagement/ChangesManager.h"
#include "ChangesManagement/JitActions.h"

namespace fs = std::filesystem;

template<class T>
std::ostream &operator<<(std::ostream &os, const std::vector<T> &v) {
    for (const auto &item: v) {
        os << item << ", ";
    }
    return os;
}

int main(int argc, char *argv[]) {
    fs::path current_dir = "./";
    const fs::path &base_dir = current_dir;
    manager::JitActions jitActions(base_dir.string() + "/.jit");

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <git-command> [args]" << std::endl;
        return 1;
    }

    std::string command = argv[1];

    // Handle different git commands using a switch-like structure with a map
    if (command == "add") {
        if (argc != 3) {
            std::cerr << "Usage: git add <filename>" << std::endl;
            return 1;
        }
        jitActions.jit_add({argv[2]});

    } else if (command == "commit") {
        if (argc != 3) {
            std::cerr << "Usage: git commit <message>" << std::endl;
            return 1;
        }
        jitActions.commit(argv[2]);

    } else if (command == "checkout" && argc == 4 && std::string(argv[2]) == "-b") {
        jitActions.create_branch(argv[3]);
    } else if (command == "checkout") {
        if (argc != 3) {
            std::cerr << "Usage: git checkout <branch-name>/address" << std::endl;
            return 1;
        }
        jitActions.checkout_to_a_commit(argv[2]);

    } else if (command == "status") {
        jitActions.print_jit_status();

    } else if (command == "log") {
        jitActions.jit_commit_log();

    } else if (command == "merge") {
        if (argc != 3) {
            std::cerr << "Usage: git merge <branch-name>" << std::endl;
            return 1;
        }
//        git_merge(argv[2]);
    } else if (command == "branches") {
//        git_branches();
    } else if (command == "init") {
        jitActions.initialize_jit();
    } else {
        std::cerr << "Unknown command: " << command << std::endl;
        return 1;
    }

    return 0;
}
