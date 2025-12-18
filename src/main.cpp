#include <iostream>
#include <stfs/device.h>
#include <stfs/fs.h>
#include <string>
#include <stfs/crypto.h>
#include <stfs/block.h>
#include <stfs/index.h>
using namespace std;

#define BLOCK_PAYLOAD 32

int main()
{
    ClusterStructsSizes sizes = {
        .index_entry_size = INDEX_ENTRY_SIZE,
        .total_block_size = BLOCK_STATIC_SIZE + BLOCK_PAYLOAD,
        .transaction_size = CLUSTER_STATE_SIZE + BLOCK_STATIC_SIZE + BLOCK_PAYLOAD};
    StorageCluster cluster(std::make_unique<Raid0>(), sizes);

    DeviceFormatter formater = [](const std::string &path, uint64_t device_head_offset, uint8_t device_id) -> std::unique_ptr<Device> {
        return FileDevice::format(path, device_head_offset, 200 ,device_id);
    };

    std::vector<DeviceFormatBlueprint> blueprints = {{"disk1.stfs", formater},{"disk2.stfs", formater},{"disk3.stfs", formater},{"disk4.stfs", formater}};

    cluster.format_cluster(blueprints, BLOCK_PAYLOAD);

    // DeviceOpener opener = [](const std::string &path, uint64_t device_head_offset) -> std::unique_ptr<Device>
    // {
    //     return FileDevice::open(path, device_head_offset);
    // };

    // std::vector<DeviceOpenBlueprint> blueprints = {{"disk1.stfs", opener}, {"disk2.stfs", opener}};
    // cluster.open_cluster(blueprints);

    Index index(cluster, 100);
    Journal journal(cluster, index);

    Fs fs(cluster, index, journal);

    for (int i = 2; i < 90; i++)
    {
        std::string data = "DISK_DATA_" + std::to_string(i) + "_TIMESTAMP";
        fs.create_block(i, data.c_str());
    }

    // while (true) {
    //     int timestamp;
    //     cin >> timestamp;
    //     auto data = fs.get_block_by_timestamp(timestamp);
    //     for (const auto &e : data.payload)
    //     {
    //         cout << e;
    //     }
    //     cout << endl;
    // }


    // auto data = fs.get_block_by_timestamp(0);

    // for (const auto& e: data.payload) {
    //     cout << e;
    // }
    // cout << endl;

    // data = fs.get_block_by_timestamp(1);

    // for (const auto& e: data.payload) {
    //     cout << e;
    // }
    // cout << endl;
}