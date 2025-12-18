#include <optional>
#include <algorithm>
#include <iterator>
#include <stfs/serelization.h>
#include <stfs/const.h>
#include <stfs/crypto.h>
#include <stfs/storage_cluster.h>

std::vector<char> ClusterHead::serialize() const
{
    std::vector<char> buffer;
    buffer.resize(CLUSTER_HEAD_SIZE);

    char *ptr = buffer.data();

    std::memcpy(ptr, magic.data(), magic.size());
    ptr += magic.size();

    SERIALIZE_FIELD(ptr, version, uint32_t, serializeU32);
    std::memcpy(ptr, &num_of_disks, sizeof(num_of_disks));
    ptr += sizeof(num_of_disks);
    std::memcpy(ptr, &raid_type, sizeof(raid_type));
    ptr += sizeof(raid_type);
    std::memcpy(ptr, mime, sizeof(mime));
    ptr += sizeof(mime);
    SERIALIZE_FIELD(ptr, block_payload_size, uint64_t, serializeU64);
    SERIALIZE_FIELD(ptr, total_blocks, uint64_t, serializeU64);
    SERIALIZE_FIELD(ptr, device_head_offset, uint64_t, serializeU64);
    SERIALIZE_FIELD(ptr, cluster_state_offset, uint64_t, serializeU64);
    SERIALIZE_FIELD(ptr, index_offset, uint64_t, serializeU64);
    SERIALIZE_FIELD(ptr, data_offset, uint64_t, serializeU64);

    SERIALIZE_FIELD(ptr, reserve1, uint64_t, serializeU64);
    SERIALIZE_FIELD(ptr, reserve2, uint64_t, serializeU64);
    SERIALIZE_FIELD(ptr, reserve3, uint64_t, serializeU64);
    SERIALIZE_FIELD(ptr, reserve4, uint64_t, serializeU64);

    SERIALIZE_FIELD(ptr, crc32, uint32_t, serializeU32);
    return buffer;
}

size_t ClusterHead::deserialize(const char *buffer)
{
    const char *start = buffer;

    std::memcpy(magic.data(), buffer, magic.size());
    buffer += magic.size();

    DESERIALIZE_FIELD(buffer, version, uint32_t, deserializeU32);
    num_of_disks = *buffer;
    buffer += sizeof(num_of_disks);
    raid_type = *buffer;
    buffer += sizeof(raid_type);
    std::memcpy(mime, buffer, sizeof(mime));
    buffer += sizeof(mime);
    DESERIALIZE_FIELD(buffer, block_payload_size, uint64_t, deserializeU64);
    DESERIALIZE_FIELD(buffer, total_blocks, uint64_t, deserializeU64);
    DESERIALIZE_FIELD(buffer, device_head_offset, uint64_t, deserializeU64);
    DESERIALIZE_FIELD(buffer, cluster_state_offset, uint64_t, deserializeU64);
    DESERIALIZE_FIELD(buffer, index_offset, uint64_t, deserializeU64);
    DESERIALIZE_FIELD(buffer, data_offset, uint64_t, deserializeU64);

    DESERIALIZE_FIELD(buffer, reserve1, uint64_t, deserializeU64);
    DESERIALIZE_FIELD(buffer, reserve2, uint64_t, deserializeU64);
    DESERIALIZE_FIELD(buffer, reserve3, uint64_t, deserializeU64);
    DESERIALIZE_FIELD(buffer, reserve4, uint64_t, deserializeU64);
    DESERIALIZE_FIELD(buffer, crc32, uint32_t, deserializeU32);

    return buffer - start;
}

void ClusterHead::update_crc()
{
    this->crc32 = 0;
    auto serialized_data = serialize();
    this->crc32 = generate_CRC32(
        reinterpret_cast<const uint8_t *>(serialized_data.data()),
        serialized_data.size());
}

bool ClusterHead::is_valid() const
{
    ClusterHead self_copy = *this;
    self_copy.crc32 = 0;
    auto serialized_data = self_copy.serialize();

    uint32_t calculated_crc = generate_CRC32(
        reinterpret_cast<const uint8_t *>(serialized_data.data()),
        serialized_data.size());

    return calculated_crc == this->crc32;
}

std::vector<char> ClusterState::serialize() const
{
    std::vector<char> buffer;
    buffer.resize(CLUSTER_STATE_SIZE);

    char *ptr = buffer.data();

    SERIALIZE_FIELD(ptr, head_logical_block_id, uint64_t, serializeU64);
    SERIALIZE_FIELD(ptr, tail_logical_block_id, uint64_t, serializeU64);
    SERIALIZE_FIELD(ptr, valid_block_count, uint64_t, serializeU64);
    SERIALIZE_FIELD(ptr, total_writes_count, uint64_t, serializeU64);
    SERIALIZE_FIELD(ptr, crc32, uint32_t, serializeU32);

    return buffer;
}

size_t ClusterState::deserialize(const char *buffer)
{
    const char *start = buffer;

    DESERIALIZE_FIELD(buffer, head_logical_block_id, uint64_t, deserializeU64);
    DESERIALIZE_FIELD(buffer, tail_logical_block_id, uint64_t, deserializeU64);
    DESERIALIZE_FIELD(buffer, valid_block_count, uint64_t, deserializeU64);
    DESERIALIZE_FIELD(buffer, total_writes_count, uint64_t, deserializeU64);
    DESERIALIZE_FIELD(buffer, crc32, uint32_t, deserializeU32);

    return buffer - start;
}

void ClusterState::update_crc()
{
    this->crc32 = 0;
    auto serialized_data = serialize();
    this->crc32 = generate_CRC32(
        reinterpret_cast<const uint8_t *>(serialized_data.data()),
        serialized_data.size());
}

bool ClusterState::is_valid() const
{
    ClusterState self_copy = *this;
    self_copy.crc32 = 0;

    auto serialized_data = self_copy.serialize();

    uint32_t calculated_crc = generate_CRC32(
        reinterpret_cast<const uint8_t *>(serialized_data.data()),
        serialized_data.size());

    return calculated_crc == this->crc32;
}

std::unique_ptr<char[]> StorageCluster::read_and_verify_mirrored_data(
    const std::vector<PhysicalAddress> &addresses,
    size_t size,
    const DataValidator &is_valid)
{
    std::map<std::vector<char>, int> vote_counts;

    for (auto &address : addresses)
    {
        try
        {
            auto bytes = read(address.disk_id, address.offset, size);

            if (is_valid(bytes.get(), size))
            {
                std::vector<char> key(bytes.get(), bytes.get() + size);
                vote_counts[key]++;
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Warning: could not read data from device " << (int)address.disk_id << ": " << e.what() << std::endl;
        }
    }

    if (vote_counts.empty())
    {
        throw ClusterError("No valid data found on any device.");
    }

    auto winner_it = std::max_element(vote_counts.begin(), vote_counts.end(),
                                      [](const auto &a, const auto &b)
                                      { return a.second < b.second; });

    int max_votes = winner_it->second;
    const std::vector<char> &winner_key = winner_it->first;

    size_t required_quorum = (addresses.size() / 2) + 1;
    if (max_votes < required_quorum)
    {
        throw ClusterError("Cluster is inconsistent: No quorum for data. Manual intervention required.");
    }

    const std::vector<char> &winning_data = winner_it->first;

    for (auto &address : addresses)
    {
        bool needs_restore = true;
        try
        {
            auto bytes = read(address.disk_id, address.offset, size);
            std::vector<char> current_key(bytes.get(), bytes.get() + size);
            if (current_key == winner_key)
            {
                needs_restore = false;
            }
        }
        catch (const std::exception &e)
        {
            needs_restore = true;
        }

        if (needs_restore)
        {
            std::cout << "Restoring metadata on device " << (int)address.disk_id << std::endl;
            write(address.disk_id, address.offset, winning_data.data(), size);
        }
    }

    auto response = std::make_unique<char[]>(size);

    std::memcpy(response.get(), winning_data.data(), size);

    return response;
}

void StorageCluster::read_and_verify_heads()
{
    DataValidator validator = [](const char *data, size_t size) -> bool
    {
        if (size != CLUSTER_HEAD_SIZE)
        {
            return false;
        }

        ClusterHead candidate;
        candidate.deserialize(data);

        return candidate.is_valid();
    };

    std::vector<PhysicalAddress> addresses;
    addresses.reserve(devices_.size());

    std::transform(
        devices_.begin(),
        devices_.end(),
        std::back_inserter(addresses),
        [](const auto &pair)
        {
            return PhysicalAddress{
                .disk_id = pair.first,
                .offset = 0};
        });

    auto stored_head = read_and_verify_mirrored_data(addresses, CLUSTER_HEAD_SIZE, validator);

    head_.deserialize(stored_head.get());
}

void StorageCluster::read_and_verify_states()
{
    DataValidator validator = [](const char *data, size_t size) -> bool
    {
        if (size != CLUSTER_STATE_SIZE)
        {
            return false;
        }

        ClusterState candidate;
        candidate.deserialize(data);

        return candidate.is_valid();
    };

    std::vector<PhysicalAddress> addresses;
    addresses.reserve(devices_.size());

    std::transform(
        devices_.begin(),
        devices_.end(),
        std::back_inserter(addresses),
        [this](const auto &pair)
        {
            return PhysicalAddress{
                .disk_id = pair.first,
                .offset = head_.cluster_state_offset};
        });

    auto stored_state = read_and_verify_mirrored_data(addresses, CLUSTER_STATE_SIZE, validator);

    state_.deserialize(stored_state.get());
}

void StorageCluster::write_head_to_all_devices()
{
    head_.update_crc();

    auto serialized = head_.serialize();

    mirrored_write(0, reinterpret_cast<const char *>(serialized.data()), CLUSTER_HEAD_SIZE);
}

void StorageCluster::write_state_to_all_devices()
{
    state_.update_crc();

    auto serialized = state_.serialize();

    mirrored_write(head_.cluster_state_offset, reinterpret_cast<const char *>(serialized.data()), CLUSTER_STATE_SIZE);
}

void StorageCluster::mirrored_write(size_t address, const char *data, size_t size)
{
    for (const auto &[id, _value] : devices_)
    {
        write(id, address, data, size);
    }
}

void StorageCluster::write(uint8_t device_id, size_t address, const char *data, size_t size)
{
    const auto &device = devices_[device_id];

    if (!device)
    {
        throw ClusterError("Device not found");
    }

    device->write(address, data, size);
}

std::unique_ptr<char[]> StorageCluster::read(uint8_t device_id, size_t address, size_t size)
{
    const auto &device = devices_[device_id];

    if (!device)
    {
        throw ClusterError("Device not found");
    }

    return device->read(address, size);
}

StorageCluster::StorageCluster(std::unique_ptr<RaidGovernor> governor, const ClusterStructsSizes &sizes) : raid_governor_(std::move(governor))
{
    index_entry_size_ = sizes.index_entry_size;
    total_block_size_ = sizes.total_block_size;
    transaction_size_ = sizes.transaction_size;
}

void StorageCluster::format_cluster(const std::vector<DeviceFormatBlueprint> &blueprints, uint64_t block_payload_size)
{
    if (blueprints.size() > MAX_DEVICES)
    {
        throw ClusterError("Device limit reached, max " + std::to_string(MAX_DEVICES));
    }

    head_.raid_type = raid_governor_->get_type();
    head_.num_of_disks = blueprints.size();
    head_.block_payload_size = block_payload_size;

    head_.total_blocks = 0;

    for (size_t i = 0; i < blueprints.size(); ++i)
    {
        const DeviceFormatBlueprint &blueprint = blueprints[i];

        auto device = blueprint.formater(blueprint.path, CLUSTER_HEAD_SIZE, i);

        head_.total_blocks += device->get_head().total_blocks_on_disk;

        devices_.emplace(i, std::move(device));
    }
    head_.data_offset = head_.index_offset + (head_.total_blocks * index_entry_size_) + transaction_size_;

    head_.update_crc();

    write_head_to_all_devices();
    write_state_to_all_devices();
}

void StorageCluster::open_cluster(const std::vector<DeviceOpenBlueprint> &blueprints)
{
    if (blueprints.size() > MAX_DEVICES)
    {
        throw ClusterError("Device limit reached, max " + std::to_string(MAX_DEVICES));
    }

    for (auto blueprint : blueprints)
    {
        auto device = blueprint.opener(blueprint.path, CLUSTER_HEAD_SIZE);

        uint8_t disk_id = device->get_head().disk_id;

        if (devices_.contains(disk_id))
        {
            throw ClusterError("Disk already exists, id: " + std::to_string(disk_id));
        }

        devices_.emplace(disk_id, std::move(device));
    }

    read_and_verify_heads();
    read_and_verify_states();
}

void StorageCluster::write_next_block(const char *data)
{
    std::vector<DiskLayout> layouts;
    layouts.reserve(devices_.size());

    std::transform(
        devices_.begin(),
        devices_.end(),
        std::back_inserter(layouts),
        [](const auto &pair)
        {
            return DiskLayout{
                .disk_id = pair.first,
                .total_blocks = pair.second->get_head().total_blocks_on_disk};
        });

    uint64_t new_block_id = get_ring_buffer_state().get_next_block_id();

    std::vector<PhysicalLocation> location_to_write = raid_governor_->map_logical_to_physical(new_block_id, layouts);

    for (PhysicalLocation location : location_to_write)
    {
        auto &device = devices_.at(location.disk_id);

        size_t offset = head_.data_offset + location.block_id_on_disk * total_block_size_;

        device->write(offset, data, total_block_size_);
    }

    state_.total_writes_count++;

    bool was_full = (state_.valid_block_count == head_.total_blocks);

    if (was_full)
    {
        state_.head_logical_block_id = (state_.head_logical_block_id + 1) % head_.total_blocks;
    }

    state_.tail_logical_block_id = new_block_id;

    if (!was_full)
    {
        state_.valid_block_count++;
    }

    write_state_to_all_devices();
}

void StorageCluster::write_index_block(uint64_t id, const char *data)
{
    size_t offset = head_.index_offset + index_entry_size_ * id;
    mirrored_write(offset, data, index_entry_size_);
}

void StorageCluster::write_transaction_block(const char *data)
{
    size_t offset = head_.index_offset + index_entry_size_ * head_.total_blocks;
    mirrored_write(offset, data, transaction_size_);
}

RingBufferState StorageCluster::get_ring_buffer_state() const
{
    return {
        .head_id = state_.head_logical_block_id,
        .tail_id = state_.tail_logical_block_id,
        .count = state_.valid_block_count,
        .capacity = head_.total_blocks};
}

std::unique_ptr<char[]> StorageCluster::read_block(uint64_t id, DataValidator validator)
{
    if (id >= head_.total_blocks)
    {
        throw ClusterError("Block id is out of bound");
    }
    std::vector<DiskLayout> layouts;
    layouts.reserve(devices_.size());

    std::transform(
        devices_.begin(),
        devices_.end(),
        std::back_inserter(layouts),
        [](const auto &pair)
        {
            return DiskLayout{
                .disk_id = pair.first,
                .total_blocks = pair.second->get_head().total_blocks_on_disk};
        });

    std::vector<PhysicalLocation> location_to_write = raid_governor_->map_logical_to_physical(id, layouts);

    std::vector<PhysicalAddress> physical_locations;
    physical_locations.reserve(location_to_write.size());

    std::transform(
        location_to_write.begin(),
        location_to_write.end(),
        std::back_inserter(physical_locations),
        [this](auto &location)
        {
            return PhysicalAddress{
                .disk_id = location.disk_id,
                .offset = head_.data_offset + (location.block_id_on_disk * total_block_size_)};
        });

    return read_and_verify_mirrored_data(physical_locations, total_block_size_, validator);
}

std::unique_ptr<char[]> StorageCluster::read_index_block(uint64_t id, DataValidator validator)
{
    if (id >= head_.total_blocks)
    {
        throw ClusterError("Block id is out of bound");
    }
    size_t entry_offset = head_.index_offset + index_entry_size_ * id;

    std::vector<PhysicalAddress> addresses;
    addresses.reserve(devices_.size());

    std::transform(
        devices_.begin(),
        devices_.end(),
        std::back_inserter(addresses),
        [entry_offset](const auto &pair)
        {
            return PhysicalAddress{
                .disk_id = pair.first,
                .offset = entry_offset};
        });

    return read_and_verify_mirrored_data(addresses, index_entry_size_, validator);
}

std::unique_ptr<char[]> StorageCluster::read_transaction_block(DataValidator validator)
{
    size_t offset = head_.index_offset + index_entry_size_ * head_.total_blocks;
    std::vector<PhysicalAddress> addresses;
    addresses.reserve(devices_.size());

    std::transform(
        devices_.begin(),
        devices_.end(),
        std::back_inserter(addresses),
        [offset](const auto &pair)
        {
            return PhysicalAddress{
                .disk_id = pair.first,
                .offset = offset};
        });
    return read_and_verify_mirrored_data(addresses, transaction_size_, validator);
}

const ClusterState &StorageCluster::get_state() const
{
    return state_;
}

const ClusterHead &StorageCluster::get_head() const
{
    return head_;
}

void StorageCluster::update_state(ClusterState state)
{
    state_ = state;
}