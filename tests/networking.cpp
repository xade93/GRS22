#include <catch2/catch_test_macros.hpp>
#include "common.tpp"
#include <cryptoTools/Network/Session.h>
#include <cryptoTools/Network/IOService.h>
#include <cryptoTools/Network/Channel.h>
using namespace osuCrypto;

// TEST_CASE("Networking", "[cryptoTools]") {
//     std::array<block, 2> vec; // = {std::bitset<5>(0b01010), std::bitset<5>(0b11111)};
//     auto thread = std::thread([&] {
//         IOService ios;
//         auto chl = Session(ios, "localhost:8081", SessionMode::Server).addChannel();
//         chl.send(std::move(vec));
//     });
// }