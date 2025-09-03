cmake -S . -B build -G Ninja -DCMAKE_LINKER=lld
cmake --build build --parallel