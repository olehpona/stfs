#include <stdexcept>
#include <stfs/device.h>
#include <stfs/fs.h>

FileDevice::FileDevice(const std::string& filename, uint64_t offset, bool is_new_file)
    : head_offset_(offset) 
{
    auto flags = std::ios::in | std::ios::out | std::ios::binary;
    if (is_new_file) {
        flags |= std::ios::trunc;
    }

    file_.open(filename, flags);
    
    if (!file_.is_open()) {
        throw DeviceError("Failed to open or create device file: " + filename);
    }
}

void FileDevice::read_head() {
    auto head_data = read(head_offset_, sizeof(DeviceHead));
    std::memcpy(&head_, head_data.get(), sizeof(DeviceHead));
}

void FileDevice::write_head() {
    write(head_offset_, reinterpret_cast<const char*>(&head_), sizeof(DeviceHead));
}

std::unique_ptr<Device> FileDevice::open(const std::string& dev_filename, uint64_t head_offset) {
    auto device = std::unique_ptr<FileDevice>(new FileDevice(dev_filename, head_offset, false));
    device->read_head();
    return device;
}

std::unique_ptr<Device> FileDevice::format(
    const std::string& dev_filename, 
    uint64_t head_offset, 
    uint64_t total_blocks_on_disk, 
    uint8_t disk_id
) {
    auto device = std::unique_ptr<FileDevice>(new FileDevice(dev_filename, head_offset, true));
    
    DeviceHead head {
        .disk_id = disk_id,
        .total_blocks_on_disk = total_blocks_on_disk
    };
    device->head_ = head;
    device->write_head();

    return device;
}

const DeviceHead& FileDevice::get_head() const {
    return head_;
}

std::unique_ptr<char[]> FileDevice::read(size_t position, size_t size)
{
    file_.seekg(position);
    char *data = new char[size];
    file_.read(data, size);
    return std::unique_ptr<char[]> (data);
}

void FileDevice::write(size_t position, const char *data, size_t size)
{
    file_.seekp(position);
    file_.write(data, size);
}


FileDevice::~FileDevice()
{
    file_.close();
}
