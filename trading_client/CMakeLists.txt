cmake_minimum_required(VERSION 3.14)
project(TradingClient VERSION 1.0.0 LANGUAGES CXX)

# Set CMake policies
cmake_policy(SET CMP0167 NEW)  # Use Boost's own CMake configuration
cmake_policy(SET CMP0048 NEW)  # Project version in project() command
cmake_policy(SET CMP0091 NEW)  # Enable MSVC_RUNTIME_LIBRARY
cmake_policy(SET CMP0148 NEW)  # Suppress deprecation warnings from external projects

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)

# Set MSYS2 paths
set(MSYS2_ROOT "C:/msys64/mingw64")
set(MSYS2_INCLUDE_DIR "${MSYS2_ROOT}/include")
set(MSYS2_LIB_DIR "${MSYS2_ROOT}/lib")

# Set Boost paths
set(BOOST_ROOT "C:/boost_1_69_0")
set(BOOST_INCLUDEDIR "${BOOST_ROOT}")
set(BOOST_LIBRARYDIR "${BOOST_ROOT}/stage/lib")

# Add Boost include and library directories
include_directories(${BOOST_INCLUDEDIR})

# Find required packages
find_package(Threads REQUIRED)

# Set OpenSSL paths for MSYS2
set(OPENSSL_ROOT_DIR "${MSYS2_ROOT}")
set(OPENSSL_INCLUDE_DIR "${MSYS2_INCLUDE_DIR}")
set(OPENSSL_LIBRARIES "${MSYS2_LIB_DIR}/libssl.dll.a" "${MSYS2_LIB_DIR}/libcrypto.dll.a")
find_package(OpenSSL REQUIRED)

# External dependencies
set(WEBSOCKETPP_ROOT "${CMAKE_SOURCE_DIR}/../matching_engine/external/websocketpp")
set(JSON_ROOT "${CMAKE_SOURCE_DIR}/../matching_engine/external/json")

# Add external dependencies
add_subdirectory(external/nlohmann_json EXCLUDE_FROM_ALL)

# Source files
set(SOURCES
    src/main.cpp
    src/TradingClient.cpp
)

set(HEADERS
    src/TradingClient.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${WEBSOCKETPP_ROOT}
    ${WEBSOCKETPP_ROOT}/websocketpp
    ${BOOST_INCLUDEDIR}
    ${JSON_ROOT}/include
    ${OPENSSL_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Threads::Threads
    "${BOOST_LIBRARYDIR}/libboost_system-mgw151-mt-x64-1_69.a"
    "${BOOST_LIBRARYDIR}/libboost_thread-mgw151-mt-x64-1_69.a"
    ws2_32
    mswsock
    crypt32
    nlohmann_json::nlohmann_json
    ${OPENSSL_LIBRARIES}
)

# Add compile definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    BOOST_ALL_NO_LIB
    BOOST_SYSTEM_STATIC_LINK
    BOOST_THREAD_STATIC_LINK
    BOOST_ERROR_CODE_HEADER_ONLY
    BOOST_SYSTEM_NO_DEPRECATED
    ASIO_HAS_BOOST
    ASIO_USE_OPENSSL
) 