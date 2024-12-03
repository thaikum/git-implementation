//
// Created by thaiku on 02/12/24.
//

#ifndef JIT_JITACTIONS_H
#define JIT_JITACTIONS_H

#include <string>
#include "ChangesManager.h"

namespace manager {

    class JitActions : public ChangesManager{
    public:
        JitActions(std::string jit_root);

        void commit(const std::string &message);

        void jit_commit_log();

        void checkout_to_a_commit(const std::string& target);

        void checkout_to_a_branch(const std::string& commit_sha);

        void create_branch(const std::string &branch_name);


    private:
        std::string jit_root;

        void update_head_file(const std::string& head);
    };

} // manager

#endif //JIT_JITACTIONS_H
