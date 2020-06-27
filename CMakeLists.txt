cmake_minimum_required(VERSION 3.16)
project(remove_boominfo)

set(CMAKE_CXX_STANDARD 20)

add_executable(remove_boominfo main.cpp)

find_package(libzippp CONFIG REQUIRED)
target_link_libraries(remove_boominfo PRIVATE libzip::libzip libzippp::libzippp)

add_library(rapidxml INTERFACE)
target_include_directories(remove_boominfo PRIVATE /rapidxml)
