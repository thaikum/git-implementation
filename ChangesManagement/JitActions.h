//
// Created by thaiku on 02/12/24.
//

#ifndef JIT_JITACTIONS_H
#define JIT_JITACTIONS_H

#include <string>
#include "../JitUtility/jit_utility.h"
#include "ChangesManager.h"

namespace fs = std::filesystem;

namespace manager {

    class JitActions : public ChangesManager {
    public:
        JitActions(std::string jit_root);

        void commit(const std::string &message);

        void jit_commit_log();

        void checkout_to_a_commit(const std::string &target);

        void checkout_to_a_branch(const std::string &commit_sha);

        std::map<std::string, std::vector<std::string>> get_changed_files_content(const std::set<std::string>& files_to_read);

        void create_branch(const std::string &branch_name);

        void merge(const std::string &feature_branch);

        void jit_diff();

    private:
        std::string jit_root;

        void update_head_file(const std::string &head);

        [[nodiscard]] std::string get_head();

        struct BranchTrees {
            std::map<std::string, std::string> base_branch;
            std::map<std::string, std::string> feature_branch;
        };

        [[nodiscard]] std::string get_branch_head(const std::string &branch_name);

        [[nodiscard]] std::string create_temp_file(const std::string &checksum);

        [[nodiscard]] static std::vector<std::string> read_file_to_vector(const std::string &filename);

        [[nodiscard]] static std::vector<std::string>
        three_way_merge(const std::vector<std::string> &base, const std::vector<std::string> &branch_1,
                        const std::vector<std::string> &branch_2);

        static void write_vector_to_file(const std::string &filename, const std::vector<std::string> &lines);

        std::string create_temp_file(const std::string &checksum, const std::string &file_name);
    };

} // manager

#endif //JIT_JITACTIONS_H
