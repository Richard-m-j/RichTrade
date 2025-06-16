cmake_minimum_required(VERSION 3.14)
project(MatchingEngine VERSION 1.0.0 LANGUAGES CXX)

# Set CMake policies
cmake_policy(SET CMP0167 NEW)  # Use Boost's own CMake configuration
cmake_policy(SET CMP0048 NEW)  # Project version in project() command
cmake_policy(SET CMP0091 NEW)  # Enable MSVC_RUNTIME_LIBRARY
cmake_policy(SET CMP0148 NEW)  # Suppress deprecation warnings from external projects

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)

# Enable pkg-config and FetchContent
find_package(PkgConfig REQUIRED)
include(FetchContent)

# Set Boost paths
set(BOOST_ROOT "C:/boost_1_69_0")
set(BOOST_INCLUDEDIR "${BOOST_ROOT}")
set(BOOST_LIBRARYDIR "${BOOST_ROOT}/stage/lib")

# Add Boost include and library directories
include_directories(${BOOST_INCLUDEDIR})

# Find required packages
find_package(Threads REQUIRED)

# External dependencies
set(WEBSOCKETPP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/external/websocketpp")
set(HTTPLIB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/external/cpp-httplib")

# Fetch and configure googletest
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)
FetchContent_MakeAvailable(googletest)

# Add external dependencies
add_subdirectory(external/nlohmann_json EXCLUDE_FROM_ALL)
add_subdirectory(external/cpp-httplib EXCLUDE_FROM_ALL)

# Source files
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
    "src/*.h"
)

# Create library
add_library(${PROJECT_NAME}_lib STATIC ${SOURCES})

# Include directories for the library
target_include_directories(${PROJECT_NAME}_lib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core
        ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
        ${WEBSOCKETPP_ROOT}
    ${WEBSOCKETPP_ROOT}/websocketpp
    ${HTTPLIB_ROOT}
    ${BOOST_INCLUDEDIR}
)

# Link libraries for the library
target_link_libraries(${PROJECT_NAME}_lib PUBLIC
    Threads::Threads
    "${BOOST_LIBRARYDIR}/libboost_system-mgw151-mt-x64-1_69.a"
    "${BOOST_LIBRARYDIR}/libboost_thread-mgw151-mt-x64-1_69.a"
        nlohmann_json::nlohmann_json 
        httplib 
)

# Add compile definitions for the library
target_compile_definitions(${PROJECT_NAME}_lib PUBLIC
    BOOST_ALL_NO_LIB
    BOOST_SYSTEM_STATIC_LINK
    BOOST_THREAD_STATIC_LINK
    BOOST_ERROR_CODE_HEADER_ONLY
    BOOST_SYSTEM_NO_DEPRECATED
    ASIO_HAS_BOOST
)

# Create main executable
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_lib ws2_32 mswsock crypt32)

# Testing
enable_testing()

# Test sources
file(GLOB TEST_SOURCES "tests/*.cpp")

# Create test executable
add_executable(test_${PROJECT_NAME} ${TEST_SOURCES})
target_link_libraries(test_${PROJECT_NAME} PRIVATE
    GTest::gtest_main
    ${PROJECT_NAME}_lib
)

# Add test
add_test(NAME ${PROJECT_NAME}_test COMMAND test_${PROJECT_NAME}) 