#include <iostream>
#include <stfs/device.h>
#include <stfs/fs.h>
#include <string>
#include <stfs/crypto.h>
#include <stfs/block.h>

using namespace std;

#define BLOCK_PAYLOAD 32

int main()
{
    ClusterStructsSizes sizes = {
        .total_block_size = BLOCK_STATIC_SIZE + BLOCK_PAYLOAD,
        .transaction_size = CLUSTER_STATE_SIZE + BLOCK_STATIC_SIZE + BLOCK_PAYLOAD};
    StorageCluster cluster(std::make_unique<Raid0>(), sizes);

    int mode = 0;
    cin >> mode;

    if (mode)
    {
        DeviceFormatter formater = [](const std::string &path, uint64_t device_head_offset, uint8_t device_id) -> std::unique_ptr<Device>
        {
            return FileDevice::format(path, device_head_offset, 200, device_id);
        };

        std::vector<DeviceFormatBlueprint> blueprints = {{"disk1.stfs", formater}, {"disk2.stfs", formater},{"disk3.stfs", formater}};

        cluster.format_cluster(blueprints, BLOCK_PAYLOAD);
    }
    else
    {
        DeviceOpener opener = [](const std::string &path, uint64_t device_head_offset) -> std::unique_ptr<Device>
        {
            return FileDevice::open(path, device_head_offset);
        };

        std::vector<DeviceOpenBlueprint> blueprints = {{"disk1.stfs", opener}, {"disk2.stfs", opener}, {"disk3.stfs", opener}};
        cluster.open_cluster(blueprints);
    }

    Journal journal(cluster);

    Fs fs(cluster, journal);

    if (mode)
    {
        for (int i = 0; i < 1; i++)
        {
            std::string data = "DISK_DATA_" + std::to_string(i) + "_TIMESTAMP";
            data.resize(BLOCK_PAYLOAD);
            fs.create_block(i, data.c_str());
        }
    }

    cout << cluster.get_head().device_head_offset << endl;
    cout << cluster.get_head().cluster_state_offset << endl;
    cout << cluster.get_head().journal_offset << endl;
    cout << cluster.get_head().data_offset << endl;

    // while (true)
    // {
    //     int timestamp;
    //     cin >> timestamp;
    //     auto data = fs.get_block_by_timestamp(timestamp);
    //     for (const auto &e : data.payload)
    //     {
    //         cout << e;
    //     }
    //     cout << endl;
    // }
}