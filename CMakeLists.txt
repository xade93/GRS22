cmake_minimum_required (VERSION 3.20)
project(garimella VERSION 0.1)

set(CMAKE_CXX_STANDARD 20) # Use C++20
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wfatal-errors") # stop compilation at first error

find_package(libOTe REQUIRED)

# load OpenSSL
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})


add_executable(garimella protocol.cpp protocol.tpp okvs.cpp main.cpp)

target_link_libraries(garimella oc::libOTe OpenSSL::SSL OpenSSL::Crypto)
