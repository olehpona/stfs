#include <stdexcept>
#include <stfs/device.h>
#include <stfs/fs.h>
#include <stfs/serelization.h>

std::vector<char> DeviceHead::serialize() const {
    std::vector<char> buffer;
    buffer.resize(DEVICE_HEAD_SIZE);

    char *ptr = buffer.data();

    SERIALIZE_FIELD(ptr, total_blocks_on_disk, uint64_t, serializeU64);
    std::memcpy(ptr, &disk_id, sizeof(disk_id));
    ptr += sizeof(disk_id);
    return buffer;
}

size_t DeviceHead::deserialize(const char *buffer) {
    const char* start = buffer;

    DESERIALIZE_FIELD(buffer, total_blocks_on_disk, uint64_t, deserializeU64);
    std::memcpy(&disk_id, buffer, sizeof(disk_id));
    buffer += sizeof(disk_id);
    
    return buffer-start;
}

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
    auto head_data = read(head_offset_, DEVICE_HEAD_SIZE);

    head_.deserialize( head_data.get());
}

void FileDevice::write_head() {
    auto serialized_data = head_.serialize();
    write(head_offset_, reinterpret_cast<const char*>(serialized_data.data()), DEVICE_HEAD_SIZE+2);
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
    auto data = std::make_unique<char[]>(size);
    file_.read(data.get(), size);
    if ((file_.fail() && !file_.eof()) || file_.bad()) {
        throw DeviceError("An error occurred while reading from device.");
    }
    
    if (file_.gcount() != size) {
        file_.clear();
        throw DeviceError("Unexpected end of file reached.");
    }
    return data;
}

void FileDevice::write(size_t position, const char *data, size_t size)
{
    file_.seekp(position);
    file_.write(data, size);
    file_.flush();

    if (file_.fail() || file_.bad()) {
        file_.clear();
        throw DeviceError("An error occurred while writing from device.");
    }
}


FileDevice::~FileDevice()
{
    file_.close();
}
