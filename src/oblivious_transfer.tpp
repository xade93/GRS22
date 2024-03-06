#include <catch2/catch_test_macros.hpp>
#include <coproto/Socket/AsioSocket.h>
#include <libOTe/Base/BaseOT.h>

// for hybrid encryption
#include <cryptoTools/Common/Defines.h>
#include <cryptoTools/Crypto/AES.h>

// for transfer of custom encrypted data
#include <cryptoTools/Network/Session.h>
#include <cryptoTools/Network/IOService.h>
#include <cryptoTools/Network/Channel.h>

#include "common.tpp"
using namespace osuCrypto;

// ports used for OT. port1 for transfer of key, port2 for transfer of ciphertext.
static std::string port1 = ":2344";
static std::string port2 = ":2345";

// aux function for AES encryption and decryption.
template<int N>
std::array<block, N> aesEncrypt(std::array<block, N> data, const block key) {
    details::AES<details::AESTypes::NI> aes(key);
    for (auto& e: data) e = aes.ecbEncBlock(e);
    return data;
}

template<int N>
std::array<block, N> aesDecrypt(std::array<block, N> data, const block key) {
    details::AESDec<details::AESTypes::NI> aes_dec(key);
    for (auto& e: data) e = aes_dec.ecbDecBlock(e);
    return data;
}

// primitive conversion tools between libOTe's block type and STL bitset.
namespace conversion_tools {

    // return last 64 bits of a bitset as uint64_t. pad topmost bits with 0 if bitset is less than 64 bits.
    // this servers as replacement for std::bitset<>::to_ulong() as it throws error when result cant fit in 64 bits.
    template<int N>
    uint64_t getTail(const std::bitset<N>& x) {
        uint64_t ret = 0;
        for (int i = 0; i < std::min(64, N); ++i) if (x[i]) ret |= (1ull << i);
        return ret;
    }

    template<int N>
    std::array<block, (N + 127) / 128> bsToBlocks(std::bitset<N> x) { 
        static_assert(sizeof(unsigned long) == 8);
        std::array<block, (N + 127) / 128> ret;
        for (uint64_t idx = 0; idx < (N + 127) / 128; ++idx) {
            auto lsb = getTail<N>(x); x >>= 64;
            auto msb = getTail<N>(x); x >>= 64;
            ret[idx] = block(msb, lsb);
        }
        return ret;
    }

    // quite ugly function. note it only works for 128 bit block and 64 bit long.
    template<int N>
    std::bitset<N> blocksToBs(const std::array<block, (N + 127) / 128>& x) {
        static_assert(sizeof(unsigned long) == 8);
        std::bitset<N> ret;
        
        for (uint64_t idx = 0; idx < N; ++idx) {
            uint64_t segment = ((block)x[idx / 128]).get<uint64_t>((idx % 128) / 64); // optimizable
            ret[idx] = (segment & (1ull << (idx % 128 % 64))) != 0; 
        }
        return ret;
    }
}

// signature of sender and receiver is different, so I have to write two functions instead of one.
// this is just a wrapper that handles networking, mostly modified from TwoChooseOne example in libOTe.
// also change the format to what we are using (std::pair, std::bitset, std::vector)
// note it seems that libOTe by default only OT over a "block" (128 bit). i.e. it does not take care of symmetric encryption part that is used to extend OT size past 128 bits.
template <typename OtExtSender, typename OtExtRecver, int BitLength, int NumItems>
void TwoChooseOne_Sender(std::string receiver_ip, const std::array<std::pair<std::bitset<BitLength>, std::bitset<BitLength>>, NumItems>& content) {
    static_assert(BitLength <= 128); // we are not using AES to hybrid encrypt, so our OT length is limited to at most 1 block (128 bits).

    const int PacketCount = (BitLength + 127) / 128; // since a block is 128 bit, we need ceil(BitLength / 128) copies of block to represent a bitset.
    using Serialised = std::array<block, PacketCount>;

    auto chl = cp::asioConnect(receiver_ip + port1, true);
    
    OtExtSender sender = OtExtSender();

    DefaultBaseOT base;
    BitVector bv(sender.baseOtCount());
    std::vector<block> baseMsg(sender.baseOtCount());
    PRNG prng(sysRandomSeed());
    bv.randomize(prng);

    cp::sync_wait(base.receive(bv, baseMsg, prng, chl));
    sender.setBaseOts(baseMsg, bv);
    
    AlignedUnVector<std::array<block, 2>> sMsgs(NumItems);
    prng.get(sMsgs.data(), sMsgs.size()); // populate random keys with random data

    cp::sync_wait(sender.send(sMsgs, prng, chl));
    cp::sync_wait(chl.flush());

    // note now transfer of Key is done. Next we need to transfer AES-encrypted data. 
    std::vector<std::array<Serialised, PacketCount * 2>> EncryptedContents(NumItems);
    for (uint64_t idx = 0; idx < NumItems; ++idx) {
        EncryptedContents[idx][0] = aesEncrypt<PacketCount>(conversion_tools::bsToBlocks<BitLength>(content[idx].first), sMsgs[idx][0]);
        EncryptedContents[idx][1] = aesEncrypt<PacketCount>(conversion_tools::bsToBlocks<BitLength>(content[idx].second), sMsgs[idx][1]);
    }

    // transfer the struct with cryptoTools wrapper
    IOService ios;
    auto chl2 = Session(ios, receiver_ip + port2, SessionMode::Server).addChannel();
    chl2.send(std::move(EncryptedContents));
}

template <typename OtExtSender, typename OtExtRecver, int BitLength, int NumItems>
std::array<std::bitset<BitLength>, NumItems> TwoChooseOne_Receiver(std::string sender_ip, const std::bitset<NumItems>& choice_) {
    static_assert(BitLength <= 128); // we are not using AES to hybrid encrypt, so our OT length is limited to at most 1 block (128 bits).
    
    const int PacketCount = (BitLength + 127) / 128;
    using Serialised = std::array<block, PacketCount>;

    auto chl = cp::asioConnect(sender_ip + port1, false);

    OtExtRecver receiver = OtExtRecver();

    DefaultBaseOT base;
    std::vector<std::array<block, 2>> baseMsg(receiver.baseOtCount());
    PRNG prng(sysRandomSeed());
    cp::sync_wait(base.send(baseMsg, prng, chl));
    receiver.setBaseOts(baseMsg);

    auto str = choice_.to_string();
    std::reverse(str.begin(), str.end()); // due to endianness TODO verify
    BitVector choice(str);

    AlignedUnVector<block> rMsgs(NumItems);
    cp::sync_wait(receiver.receive(choice, rMsgs, prng, chl));
    cp::sync_wait(chl.flush());

    // now having received key via OT, we decrypt the respective choices.

    std::vector<std::array<Serialised, 2>> EncryptedContents(NumItems);
    // receive the struct with cryptoTools wrapper
    IOService ios;
    auto chl2 = Session(ios, sender_ip + port2, SessionMode::Client).addChannel();
    chl2.recv(EncryptedContents);

    std::array<std::bitset<BitLength>, NumItems> ret;
    for (uint64_t idx = 0; idx < NumItems; ++idx) ret[idx] = conversion_tools::blocksToBs<BitLength>(
        aesDecrypt<PacketCount>(
            EncryptedContents[idx][choice_[idx]], 
            rMsgs[idx]
        )
    );

    return ret;
}