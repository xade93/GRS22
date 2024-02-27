#include <catch2/catch_test_macros.hpp>
#include <coproto/Socket/AsioSocket.h>
#include <libOTe/Base/BaseOT.h>
#include <libOTe/TwoChooseOne/Iknp/IknpOtExtReceiver.h>
#include <libOTe/TwoChooseOne/Iknp/IknpOtExtSender.h>
#include "common.tpp"
using namespace osuCrypto;

// primitive conversion tools between libOTe's block type and STL bitset.
namespace conversion_tools {
    const size_t BLOCK_SIZE = 128; // this is determined by block type of libOTe
    template<int N>
    block bsToBlock(std::bitset<N> x) { // pass by val by design
        static_assert(N <= BLOCK_SIZE);
        static_assert(sizeof(unsigned long) == 8);
        return block((x >> 64).to_ulong(), x.to_ulong());
    }

    template<int N>
    std::bitset<N> blockToBs(const block& x) {
        static_assert(N <= BLOCK_SIZE);    
        std::bitset<N> ret;

        for (uint64_t idx = 0; idx < N; ++idx) {
            uint64_t segment = x.get<uint64_t>(idx / 64); // optimizable
            ret[idx] = (segment & (1ull << (idx % 64))) != 0;
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
    auto chl = cp::asioConnect(receiver_ip, true);

    OtExtSender sender = OtExtSender();

    DefaultBaseOT base;
    BitVector bv(sender.baseOtCount());
    std::vector<block> baseMsg(sender.baseOtCount());
    PRNG prng(sysRandomSeed());
    bv.randomize(prng);

    cp::sync_wait(base.receive(bv, baseMsg, prng, chl));
    sender.setBaseOts(baseMsg, bv);
    
    AlignedUnVector<std::array<block, 2>> sMsgs(NumItems);
    
    for (uint64_t idx = 0; idx < NumItems; ++idx) {
        sMsgs[idx][0] = conversion_tools::bsToBlock<BitLength>(content[idx].first);
        sMsgs[idx][1] = conversion_tools::bsToBlock<BitLength>(content[idx].second);
    }

    cp::sync_wait(sender.sendChosen(sMsgs, prng, chl));
    cp::sync_wait(chl.flush());
}

template <typename OtExtSender, typename OtExtRecver, int BitLength, int NumItems>
std::array<std::bitset<BitLength>, NumItems> TwoChooseOne_Receiver(std::string sender_ip, const std::bitset<NumItems>& choice_) {
    static_assert(BitLength <= 128); // we are not using AES to hybrid encrypt, so our OT length is limited to at most 1 block (128 bits).
    auto chl = cp::asioConnect(sender_ip, false);

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
    cp::sync_wait(receiver.receiveChosen(choice, rMsgs, prng, chl));
    cp::sync_wait(chl.flush());

    std::array<std::bitset<BitLength>, NumItems> ret;
    for (uint64_t i = 0; i < NumItems; ++i) ret[i] = conversion_tools::blockToBs<BitLength>(rMsgs[i]);
    return ret;
}