#pragma once
#include <stfs/storage_cluster.h>
#include <stfs/block.h>
#include <optional>
#include <vector>

struct Transaction {
    ClusterState state;
    Block block;

    std::vector<char> serialize() const;
    size_t deserialize(const char* buffer);

    bool is_valid() const;
    void update_crc();
};

class Journal {
    private:
        StorageCluster& cluster_;
        std::optional<Transaction> entry_;
    public:
        Journal(StorageCluster& cluster);

        void create_transaction(Block block);
        void commit_transaction();
        void recover_transaction();
};