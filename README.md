![Status](https://img.shields.io/badge/status-in%20development-yellow)
![License](https://img.shields.io/badge/license-MIT-green)
![C++](https://img.shields.io/badge/C++-20-blue)
# STFS - Stream FS
STFS - is simple C++ journalized fs created for storing streams with a focus on maintaining data integrity by supporting raid, data verification, transactions and implementing possibility to easily restore data in any conditions.

## Features
- **Fast data search** – O(log n) complexity
- **Cycling data storage** for continuous streams
- **CRC32 with hardware acceleration** for integrity check
- **RAID abstraction layer**
- **Data blocks auto repair**
- **All or nothing with journalizing**
- **Device abstraction layer** - works with files, RAM, and other possible storage backends

## Layout
- **ClusterHead** ( fs head ) - stores all info about fs
- **ClusterState** - current fs state ( last block id, valid blocks, write count)
- **DeviceHead**  ( optional to store by device realization ) - stores info about device (disk_id, total_block_space)
- **Index** - Ring buffer on index entries with size of fs total data blocks count
- **Journal** - stores fs state before transaction ( cluster head, state ) and new block info
- **Data** - Ring buffer for data blocks

## Technology
- C++20 STL
- CRC32