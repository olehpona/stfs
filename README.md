![Status](https://img.shields.io/badge/status-in%20development-yellow)
![License](https://img.shields.io/badge/license-MIT-green)
![C++](https://img.shields.io/badge/C++-20-blue)
# STFS - Stream FS
STFS - is simple C++ fs created for storing streams with a focus on maintaining data integrity by supporting raid, data verification and implementing possibility to easily restore data in any conditions.

## Features
- ⚡ **Fast data search** – O(log n) complexity
- 🔄 **Cycling data storage** for continuous streams
- 🔒 **CRC32 with hardware acceleration** for integrity check
- 🛠 **RAID abstraction layer**
- 📟 **Device abstraction layer** - works with files, RAM, and other possible storage backends

## Technology
- C++20 STL
- CRC32