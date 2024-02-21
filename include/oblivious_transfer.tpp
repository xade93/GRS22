// modified from libOTe TODO courtesy
#include <catch2/catch_test_macros.hpp>
#include <coproto/Socket/AsioSocket.h>
#include <libOTe/Base/BaseOT.h>
#include <libOTe/TwoChooseOne/Iknp/IknpOtExtReceiver.h>
#include <libOTe/TwoChooseOne/Iknp/IknpOtExtSender.h>
#include "common.tpp"
using namespace osuCrypto;

void noHash(IknpOtExtSender& s, IknpOtExtReceiver& r);

enum Role { Receiver, Sender };

const bool isVerbose = false;

// signature of sender and receiver is different, so I have to write two functions instead of one.
// this is just a wrapper that handles networking, mostly modified from TwoChooseOne example in libOTe.
// also change the format to what we are using (std::pair, std::bitset, std::vector)
template <typename OtExtSender, typename OtExtRecver, int BitLength, int NumItems>
void TwoChooseOne_Sender(std::string receiver_ip, const std::array<std::pair<std::bitset<BitLength>, std::bitset<BitLength>>, NumItems>& content) {
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
    // TODO convert 
    cp::sync_wait(sender.sendChosen(sMsgs, prng, chl));
    cp::sync_wait(chl.flush());
}

template <typename OtExtSender, typename OtExtRecver, int BitLength, int NumItems>
std::array<std::bitset<BitLength>, NumItems> TwoChooseOne_Receiver(std::string sender_ip, const std::bitset<NumItems>& choice_) {
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
    for (uint64_t i = 0; i < NumItems; ++i) ret[i] = rMsgs[i];
}