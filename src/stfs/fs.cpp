#include <stfs/fs.h>
#include <stfs/search_engine.h>
#include <iostream>

Fs::Fs(StorageCluster &cluster_ref, Journal &journal_ref) : cluster_(cluster_ref), journal_(journal_ref)
{
    try {
        journal_.recover_transaction();
    } catch (...) {
        std::cerr << "Recovering transaction failed, skiping" << std::endl; 
    }
}

void Fs::create_block(uint64_t timestamp, const char *payload)
{
    uint64_t payload_size = cluster_.get_head().block_payload_size;

    std::vector<char> data;
    data.assign(payload, payload + payload_size);

    Block block = {timestamp, payload_size, data, 0};

    add_block(block);
}
void Fs::add_block(Block block)
{
    block.update_crc();
    auto serialized = block.serialize();
    journal_.create_transaction(block);
    journal_.commit_transaction();
}

Block Fs::read_block(uint64_t id)
{
    DataValidator validator = [](const char *data, size_t size) -> bool
    {
        Block block;
        block.deserialize(data);

        return block.is_valid();
    };

    Block block;

    auto data = cluster_.read_block(id, validator);
    block.deserialize(data.get());

    return block;
}

Block Fs::get_block_by_id(uint64_t id)
{
    return read_block(id);
}
Block Fs::get_block_by_timestamp(uint64_t timestamp)
{
    TimeStampFetcher fetcher = [this](uint64_t id) -> uint64_t {
        return read_block(id).timestamp;
    };
    auto block_id = SearchEngine::find_block_id_by_timestamp(timestamp, fetcher, cluster_.get_ring_buffer_state());
    return read_block(block_id);
}