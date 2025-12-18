#include <stfs/journal.h>
#include <stfs/serelization.h>
#include <stfs/crypto.h>

std::vector<char> Transaction::serialize() const
{
    auto serialized_state = state.serialize();
    auto serialized_block = block.serialize();

    uint8_t size = serialized_state.size() + serialized_block.size();

    std::vector<char> data;
    data.resize(size);

    char *ptr = data.data();

    std::memcpy(ptr, serialized_state.data(), serialized_state.size());
    ptr += serialized_state.size();
    std::memcpy(ptr, serialized_block.data(), serialized_block.size());

    return data;
}

size_t Transaction::deserialize(const char *buffer)
{
    const char *start = buffer;

    size_t state_size = state.deserialize(buffer);
    buffer += state_size;

    size_t block_size = block.deserialize(buffer);
    buffer += block_size;

    return buffer - start;
}

bool Transaction::is_valid() const
{
    return state.is_valid() && block.is_valid();
}

void Transaction::update_crc() {
    block.update_crc();
    state.update_crc();
};
Journal::Journal(StorageCluster& cluster, Index& index): cluster_(cluster), index_(index) {}

void Journal::create_transaction(Block block) {
    Transaction transaction = {
        .state = cluster_.get_state(),
        .block = block
    };

    entry_ = transaction;
    entry_->update_crc();

    auto serialized_data = entry_->serialize();

    cluster_.write_transaction_block(serialized_data.data());
}

void Journal::commit_transaction() {
    auto serialized_block = entry_->block.serialize();
    cluster_.write_next_block(serialized_block.data());
    uint64_t last_block_id = cluster_.get_ring_buffer_state().tail_id;
    IndexEntry entry = {
        .timestamp = entry_->block.timestamp
    };
    index_.modify_entry(last_block_id, entry);

    size_t struct_size = entry_->serialize().size();

    std::vector<char> zero_vector(struct_size, 0);

    cluster_.write_transaction_block(zero_vector.data());
}

void Journal::recover_transaction() {
    DataValidator validator = [](const char *data, size_t size) -> bool {
        Transaction transaction;
        transaction.deserialize(data);

        return transaction.is_valid();
    };

    auto data = cluster_.read_transaction_block(validator);
    Transaction transaction;
    transaction.deserialize(data.get());
    entry_ = transaction;
    commit_transaction();
}