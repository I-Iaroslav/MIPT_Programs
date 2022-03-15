cmake_minimum_required(VERSION 3.16)

project(udp_test VERSION 1.0.0.0 LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(CMAKE_CXX_STANDARD 17)

find_package(OpenSSL REQUIRED)


add_executable(sender sender.cpp)
add_executable(receiver receiver.cpp)

target_link_libraries(sender OpenSSL::Crypto)
target_link_libraries(receiver OpenSSL::Crypto)
