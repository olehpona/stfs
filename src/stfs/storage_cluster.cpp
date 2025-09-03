#include <optional>
#include <algorithm>
#include <iterator>
#include <stfs/const.h>
#include <stfs/crypto.h>
#include <stfs/storage_cluster.h>

void ClusterHead::update_crc()
{
    this->crc32 = 0;
    this->crc32 = generate_CRC32(
        reinterpret_cast<const uint8_t *>(this),
        sizeof(*this));
}

bool ClusterHead::is_valid() const
{
    ClusterHead self_copy = *this;
    self_copy.crc32 = 0;

    uint32_t calculated_crc = generate_CRC32(
        reinterpret_cast<const uint8_t *>(&self_copy),
        sizeof(self_copy));

    return calculated_crc == this->crc32;
}

void ClusterState::update_crc()
{
    this->crc32 = 0;
    this->crc32 = generate_CRC32(
        reinterpret_cast<const uint8_t *>(this),
        sizeof(*this));
}

bool ClusterState::is_valid() const {
    ClusterState self_copy = *this;
    self_copy.crc32 = 0;

    uint32_t calculated_crc = generate_CRC32(
        reinterpret_cast<const uint8_t *>(&self_copy),
        sizeof(self_copy));

    return calculated_crc == this->crc32;
}

std::unique_ptr<char[]> StorageCluster::read_and_verify_mirrored_data(
    const std::vector<uint8_t> &device_ids,
    uint64_t offset,
    size_t size,
    const DataValidator &is_valid)
{
    std::map<std::vector<char>, int> vote_counts;

    for (auto& device_id: device_ids)
    {
        auto& device = devices_.at(device_id);
        try
        {
            auto bytes = device->read(offset, size);

            if (is_valid(bytes.get(), size))
            {
                std::vector<char> key(bytes.get(), bytes.get() + size);
                vote_counts[key]++;
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Warning: could not read data from device " << (int)device_id << ": " << e.what() << std::endl;
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

    size_t required_quorum = (devices_.size() / 2) + 1;
    if (max_votes < required_quorum)
    {
        throw ClusterError("Cluster is inconsistent: No quorum for data. Manual intervention required.");
    }

    const std::vector<char> &winning_data = winner_it->first;

    for (auto& device_id: device_ids)
    {
        auto& device = devices_.at(device_id);
        bool needs_restore = true;
        try
        {
            auto bytes = device->read(offset, size);
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
            std::cout << "Restoring metadata on device " << (int)device_id << std::endl;
            write(device_id, offset, winning_data.data(), size);
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
        if (size != sizeof(ClusterHead))
        {
            return false;
        }

        ClusterHead candidate;
        std::memcpy(&candidate, data, size);

        return candidate.is_valid();
    };

    std::vector<uint8_t> devices_ids;
    devices_ids.reserve(devices_.size());

    std::transform(
        devices_.begin(),
        devices_.end(),
        std::back_inserter(devices_ids),
        [](const auto& pair) {
            return pair.first;
        }
    );

    auto stored_head = read_and_verify_mirrored_data(devices_ids, 0, sizeof(ClusterHead), validator);

    std::memcpy(&head_, stored_head.get(), sizeof(ClusterHead));
}

void StorageCluster::read_and_verify_states()
{
    DataValidator validator = [](const char *data, size_t size) -> bool
    {
        if (size != sizeof(ClusterState))
        {
            return false;
        }

        ClusterState candidate;
        std::memcpy(&candidate, data, size);

        return candidate.is_valid();
    };

    std::vector<uint8_t> devices_ids;
    devices_ids.reserve(devices_.size());

    std::transform(
        devices_.begin(),
        devices_.end(),
        std::back_inserter(devices_ids),
        [](const auto& pair) {
            return pair.first;
        }
    );

    auto stored_head = read_and_verify_mirrored_data(devices_ids, sizeof(ClusterHead), sizeof(ClusterState), validator);

    std::memcpy(&state_, stored_head.get(), sizeof(ClusterState));
}

void StorageCluster::write_head_to_all_devices()
{
    head_.update_crc();
    mirrored_write(0, reinterpret_cast<const char *>(&head_), sizeof(ClusterHead));
}

void StorageCluster::write_state_to_all_devices()
{
    state_.update_crc();
    mirrored_write(sizeof(ClusterHead), reinterpret_cast<const char *>(&state_), sizeof(ClusterState));
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

explicit StorageCluster::StorageCluster(std::unique_ptr<RaidGovernor> governor) : raid_governor_(std::move(governor)) {}

void StorageCluster::format_cluster(const std::vector<DeviceFormatBlueprint> &blueprints, const ClusterConfig &config)
{
    if (blueprints.size() > MAX_DEVICES)
    {
        throw ClusterError("Device limit reached, max " + MAX_DEVICES);
    }

    index_entry_size_ = config.index_entry_size;
    total_block_size_ = config.total_block_size;

    head_.raid_type = raid_governor_->get_type();
    head_.num_of_disks = blueprints.size();
    head_.block_payload_size = config.block_payload_size;

    head_.total_blocks = 0;

    for (size_t i = 0; i < blueprints.size(); ++i)
    {
        const DeviceFormatBlueprint &blueprint = blueprints[i];

        auto device = blueprint.formater(blueprint.path, sizeof(ClusterHead), i);

        head_.total_blocks += device->get_head().total_blocks_on_disk;

        devices_.emplace(i, std::move(device));
    }

    head_.update_crc();

    write_head_to_all_devices();
}

void StorageCluster::open_cluster(const std::vector<DeviceOpenBlueprint> &blueprints)
{
    if (blueprints.size() > MAX_DEVICES)
    {
        throw ClusterError("Device limit reached, max " + MAX_DEVICES);
    }

    for (auto blueprint : blueprints)
    {
        auto device = blueprint.opener(blueprint.path, sizeof(ClusterHead));

        uint8_t disk_id = device->get_head().disk_id;

        if (devices_.contains(disk_id))
        {
            throw ClusterError("Disk already exists, id: " + disk_id);
        }

        devices_.emplace(disk_id, std::move(device));
    }

    read_and_verify_heads();
}

BlockPlacement StorageCluster::write_next_block(const char *data)
{
    std::vector<DiskLayout> layouts;

    for (auto &[key, device] : devices_)
    {
        layouts.push_back(DiskLayout{
            .disk_id = key,
            .total_blocks = device->get_head().total_blocks_on_disk});
    }

    uint64_t new_block_id = state_.tail_logical_block_id;

    if (new_block_id == head_.total_blocks)
    {
        new_block_id = 0;
    }
    else
    {
        new_block_id++;
    }

    std::vector<PhysicalLocation> location_to_write = raid_governor_->map_logical_to_physical(new_block_id, layouts);

    for (PhysicalLocation location : location_to_write)
    {
        auto &device = devices_.at(location.disk_id);

        uint64_t tail_physical_id = device->get_head().tail_block_id;
        size_t offset = head_.index_offset + (index_entry_size_ * head_.total_blocks) + tail_physical_id * total_block_size_;

        device->write(offset, data, total_block_size_);

        if (tail_physical_id == device->get_head().total_blocks_on_disk)
        {
            device->update_tail_block_id(0);
        }
        else
        {
            device->update_tail_block_id(++tail_physical_id);
        }
    }

    state_.tail_logical_block_id = new_block_id;
    state_.total_writes_count++;
    if (state_.valid_block_count < head_.total_blocks)
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

uint64_t StorageCluster::get_total_blocks_count() const {
    return head_.total_blocks; }

std::unique_ptr<char[]> StorageCluster::read_block(uint64_t id, DataValidator validator)
{
    size_t offset = head_.index_offset + index_entry_size_ * head_.total_blocks + id * total_block_size_;
    return read_and_verify_mirrored_data(offset, total_block_size_, validator);
}

std::unique_ptr<char[]> StorageCluster::read_index_block(uint64_t id, DataValidator validator)
{
    size_t offset = head_.index_offset + index_entry_size_ * id;
    return read_and_verify_mirrored_data(offset, index_entry_size_, validator);
}