cmake_minimum_required (VERSION 3.20)
project(grs22 VERSION 0.1)

set(CMAKE_CXX_STANDARD 20) # Use C++20
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Quality of Life Options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wfatal-errors") # stop compilation at first error
add_compile_options(-Wno-deprecated-declarations)

# libraries preparation
find_package(libOTe REQUIRED)
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

# seek files to compile
file(GLOB SOURCES "src/*.cpp")
file(GLOB TESTS "tests/*.cpp")

# compile main target
add_executable(grs22 ${SOURCES} main.cpp)

target_include_directories(grs22 PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(grs22 oc::libOTe OpenSSL::SSL OpenSSL::Crypto)

# compile test target
find_package(Catch2 3 REQUIRED)

add_executable(tests ${SOURCES} ${TESTS})
target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain oc::libOTe OpenSSL::SSL OpenSSL::Crypto)