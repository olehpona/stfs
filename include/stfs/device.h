#pragma once
#include <iostream>
#include <cstdint>
#include <fstream>
#include <string>
#include <stdexcept>

struct DeviceHead {
    uint64_t total_blocks_on_disk = 0;
    uint8_t  disk_id = 0;
};

class DeviceError : public std::runtime_error
{
public:
    DeviceError(const std::string &msg) : std::runtime_error(msg) {}
};

class Device {
    public:
        virtual const DeviceHead& get_head() const = 0;
        virtual void write(size_t position, const char* data, size_t size) = 0;
        virtual std::unique_ptr<char[]> read(size_t position, std::size_t size) = 0;
        virtual ~Device() = default;
};

class FileDevice: public Device {
    private:
        std::fstream file_;
        uint64_t head_offset_;
        DeviceHead head_;
        
        FileDevice(const std::string& filename, uint64_t offset, bool is_new_file);
        void read_head();
        void write_head();
    public:
        static std::unique_ptr<Device> open(const std::string& dev_filename, uint64_t head_offset);
        static std::unique_ptr<Device> format(const std::string& filename, uint64_t head_offset, uint64_t total_blocks_on_disk, uint8_t disk_id);
        const DeviceHead& get_head() const override;
        void write(size_t position, const char* data, size_t size) override;
        std::unique_ptr<char[]> read(size_t position, std::size_t size) override;
        ~FileDevice();
};