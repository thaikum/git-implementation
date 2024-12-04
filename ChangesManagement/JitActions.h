//
// Created by thaiku on 02/12/24.
//

#ifndef JIT_JITACTIONS_H
#define JIT_JITACTIONS_H

#include <string>
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

        void create_branch(const std::string &branch_name);

        void merge(const std::string &feature_branch);

    private:
        std::string jit_root;

        void update_head_file(const std::string &head);

        std::string get_head();

        struct BranchTrees {
            std::map<std::string, std::string> base_branch;
            std::map<std::string, std::string> feature_branch;
        };

        std::string get_branch_head(const std::string &branch_name);

        std::string create_temp_file(const std::string& checksum);

        static std::vector<std::string> read_file_to_vector(const std::string &filename);

        static std::vector<std::string>
        three_way_merge(const std::vector<std::string> &base, const std::vector<std::string> &branch_1,
                        const std::vector<std::string> &branch_2);

        static void write_vector_to_file(const std::string &filename, const std::vector<std::string>& lines);
    };

} // manager

#endif //JIT_JITACTIONS_H
