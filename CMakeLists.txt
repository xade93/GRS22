cmake_minimum_required (VERSION 3.15)
project(garimella VERSION 0.1)

find_package(libOTe REQUIRED)

add_executable(garimella protocol.cpp protocol.tpp main.cpp)
target_link_libraries(garimella oc::libOTe)