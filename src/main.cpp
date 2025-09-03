#include <iostream>
#include <stfs/device.h>
#include <stfs/fs.h>
#include <string>
#include <stfs/crypto.h>
using namespace std;

int main(){
    //FileDevice device("./disk.stfs");
    //device.createDeviceHead(10485760, 209715, "plain/text");
    //device.readDeviceHead();
    //device.getDeviceHead().print();

    //Index index(device.getDeviceHead().total_blocks, sizeof(DeviceHead));

    //index.writeIndex(&device);
    //index.readIndex(&device);
    //index.print();

    uint8_t *data = new uint8_t[5];
    data[0] = 65;
    data[1] = 4;
    data[2] = 128;
    data[3] = 220;
    data[4] = 0;

    
    uint32_t crc32 = generate_CRC32(data, 5);
    cout << crc32 << endl;


    cout << validate_CRC32(data, crc32, 5) << endl;
}