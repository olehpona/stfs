#pragma once
#include <iostream>
#include <cstdint>
#include <stfs/block.h>
#include <stfs/storage_cluster.h>
#include <stfs/journal.h>
#include <vector>

class Fs{
    private:
        StorageCluster& cluster_;
        Journal& journal_;

        Block read_block(uint64_t id);
    public:
        Fs(StorageCluster& cluster_ref, Journal& journal_ref);
        void  create_block(uint64_t timestamp, const char *payload);
        void  add_block(Block block);
        Block get_block_by_id(uint64_t id);
        Block get_block_by_timestamp(uint64_t timestamp);
};