cmake_minimum_required(VERSION 3.20)
project(STFS VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(STFS_ENABLE_NATIVE_OPTIMIZATIONS "Enable architecture-specific optimizations (-march=native)" ON)

if(STFS_ENABLE_NATIVE_OPTIMIZATIONS)
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)

    if(COMPILER_SUPPORTS_MARCH_NATIVE)
        message(STATUS "Native optimizations enabled (-march=native)")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
    else()
        message(WARNING "-march=native is not supported by this compiler. Using generic build.")
    endif()
endif()

option(STFS_FORCE_SOFTWARE_CRC "Force software CRC32 implementation" OFF)

if(STFS_FORCE_SOFTWARE_CRC)
    message(WARNING "Forcing software CRC32")
    add_compile_definitions(FORCE_SOFTWARE_CRC)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


file(GLOB_RECURSE FS_SOURCES CONFIGURE_DEPENDS "src/stfs/*.cpp")

add_library(stfs_lib ${FS_SOURCES})

target_include_directories(stfs_lib
    PUBLIC ${PROJECT_SOURCE_DIR}/include
)


add_executable(STFS src/main.cpp)
target_link_libraries(STFS PRIVATE stfs_lib)