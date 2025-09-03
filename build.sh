#!/bin/bash
set -e

BUILD_DIR="build"

COMMAND=$1
BUILD_TYPE="Debug"
if [[ "$COMMAND" == "release" || "$COMMAND" == "install" ]]; then
    BUILD_TYPE="Release"
fi

if [ ! -d "$BUILD_DIR" ]; then
    echo "--- Configuring CMake for the first time ---"
    cmake -S . -B $BUILD_DIR -G "Ninja Multi-Config" \
          -DCMAKE_LINKER=lld \
          -DCMAKE_INSTALL_PREFIX=./install
fi

if [[ "$COMMAND" == "install" ]]; then
    echo "--- Building and Installing (Release) ---"
    cmake --build $BUILD_DIR --config $BUILD_TYPE --target install --parallel
    echo "--- Library installed to ./install directory ---"
else
    # Якщо команда не вказана, за замовчуванням буде "debug"
    if [[ "$COMMAND" != "release" ]]; then
        COMMAND="debug"
    fi
    echo "--- Building ($BUILD_TYPE) ---"
    cmake --build $BUILD_DIR --config $BUILD_TYPE --parallel
    echo "--- Build finished for $COMMAND mode ---"
    echo "Artifacts are in: $BUILD_DIR/$BUILD_TYPE/"
fi