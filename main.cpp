#include <iostream>
#include <vector>
#include <unordered_map>
#include "DirectoryManagent/DirManager.h"
#include "ChangesManagement/JitActions.h"

namespace fs = std::filesystem;

// Helper function for argument validation
bool validate_args(int argc, int expected_args, const std::string &usage_message) {
    if (argc != expected_args) {
        std::cerr << usage_message << std::endl;
        return false;
    }
    return true;
}

// Helper function to map commands to actions
void execute_command(const std::string &command, int argc, char *argv[], manager::DirManager &dirManager) {

    if (command == "init") {
        dirManager.initialize_jit();
    } else {
        manager::JitActions jitActions(dirManager.get_root_directory());

        if (command == "add" && validate_args(argc, 3, "Usage: jit add <filename>")) {
            jitActions.jit_add({argv[2]});

        } else if (command == "commit" && validate_args(argc, 3, "Usage: jit commit <message>")) {
            jitActions.commit(argv[2]);
        } else if (command == "checkout" && argc == 4 && std::string(argv[2]) == "-b") {
            jitActions.create_branch(argv[3]);
        } else if (command == "checkout" && validate_args(argc, 3, "Usage: jit checkout <branch-name>/address")) {
            jitActions.checkout_to_a_commit(argv[2]);
        } else if (command == "status") {
            jitActions.print_jit_status();
        } else if (command == "log") {
            jitActions.jit_commit_log();
        } else if (command == "merge" && validate_args(argc, 3, "Usage: jit merge <branch-name>")) {
            jitActions.merge(argv[2]);
        } else if (command == "branch") {
            jitActions.list_jit_branches();
        } else if (command == "diff") {
            if (argc == 2) {
                jitActions.jit_diff();
            } else if (argc == 3) {
                jitActions.jit_diff(argv[2]);
            } else {
                std::cerr << "Usage: jit diff" << std::endl;
            }
        } else if (command == "clone") {
            if (argc == 3) {
                jitActions.jit_clone(argv[2]);
            } else if (argc == 5) {
                jitActions.jit_branch_clone(argv[3], argv[4], -1);
            }
        } else {
            std::cerr << "Unknown command: " << command << std::endl;
        }
    }

}

int main(int argc, char *argv[]) {
    fs::path current_dir = "./";
    const fs::path &base_dir = current_dir;

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <jit-command> [args]" << std::endl;
        return 1;
    }

    std::string command = argv[1];

    try {
        manager::DirManager dirManager(base_dir.string());

        // Execute the command
        execute_command(command, argc, argv, dirManager);

    } catch (std::exception &ex) {
        std::cerr << ex.what() << std::endl;
    }

    return 0;
}
