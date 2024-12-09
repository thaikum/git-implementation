//
// Created by thaiku on 08/12/24.
//

#ifndef JIT_COMMITGRAPH_H
#define JIT_COMMITGRAPH_H

#include <unordered_map>
#include <memory>
#include "commit.h"
#include "../DirectoryManagent/DirManager.h"


namespace manager {
    class CommitGraph{
    public:
        explicit CommitGraph(std::string commit_file_path);

        void add_commit(const Commit &commit);

        void add_commit(Commit commit, const std::vector<std::string> &parents);

        std::shared_ptr<Commit> get_commit(const std::string &checksum);

        std::shared_ptr<Commit> get_intersection_commit(const std::string &commit1, const std::string &commit2);

        void print_commit_history(std::string checksum) const;

        void save_commits(const std::string &file_path);

        void load_commits(const std::string &file_path);

    private:
        std::unordered_map<std::string, Commit> commits;
        std::string commit_file_path;
    };


}

#endif //JIT_COMMITGRAPH_H
