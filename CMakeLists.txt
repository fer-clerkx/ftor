cmake_minimum_required(VERSION 3.22)
project(torrent_project)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
)
FetchContent_MakeAvailable(googletest)

find_package(OpenSSL)

add_subdirectory(lib/CxxUrl)

enable_testing()

add_executable(
    ftor_test
    src/bencode.cpp
    test/test_bencode.cpp
    src/metainfo.cpp
    test/test_metainfo.cpp
)
target_link_libraries(ftor_test GTest::gtest_main)
target_link_libraries(ftor_test OpenSSL::Crypto)
target_link_libraries(ftor_test chmike::CxxUrl)

include(GoogleTest)
gtest_discover_tests(ftor_test)
