#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include <coproto/Socket/AsioSocket.h>
#include <cryptoTools/Common/Log.h>  // for lout
#include <libOTe/Base/BaseOT.h>
#include <libOTe/TwoChooseOne/Iknp/IknpOtExtReceiver.h>
#include <libOTe/TwoChooseOne/Iknp/IknpOtExtSender.h>
using namespace osuCrypto;

// HACK enable iknp
void noHash(IknpOtExtSender& s, IknpOtExtReceiver& r);

// HACK add roles
enum Role { Receiver, Sender };

const bool isVerbose = false;

// HACK removed multithreading & fake OT
template <typename OtExtSender, typename OtExtRecver>
void TwoChooseOneProtocol(Role role, int totalOTs, int numThreads,
                          std::string ip, std::string tag) {
    if (totalOTs == 0) totalOTs = 1 << 2;

    bool randomOT = false; // modify to true OT

    // get up the networking
    auto chl = cp::asioConnect(ip, role == Role::Sender);

    PRNG prng(sysRandomSeed());

    OtExtSender sender = OtExtSender();
    OtExtRecver receiver = OtExtRecver();

    // HACK ifdef has base OT
    // Now compute the base OTs, we need to set them on the first pair of
    // extenders. In real code you would only have a sender or reciever, not
    // both. But we do here just showing the example.
    if (role == Role::Receiver) {
        DefaultBaseOT base;
        std::vector<std::array<block, 2>> baseMsg(receiver.baseOtCount());

        // perform the base To, call sync_wait to block until they have
        // completed.
        cp::sync_wait(base.send(baseMsg, prng, chl));
        receiver.setBaseOts(baseMsg);
    } else {
        DefaultBaseOT base;
        BitVector bv(sender.baseOtCount());
        std::vector<block> baseMsg(sender.baseOtCount());
        bv.randomize(prng);

        // perform the base To, call sync_wait to block until they have
        // completed.
        cp::sync_wait(base.receive(bv, baseMsg, prng, chl));
        sender.setBaseOts(baseMsg, bv);
    }

    // if (cmd.isSet("noHash")) noHash(sender, receiver); HACK

    Timer timer, sendTimer, recvTimer;
    sendTimer.setTimePoint("start");
    recvTimer.setTimePoint("start");
    auto s = timer.setTimePoint("start");

    if (role == Role::Receiver) {
        // construct the choices that we want.
        BitVector choice(totalOTs);

        // in this case pick random messages.
        choice.randomize(prng);

        std::cout << "Choices: ";
        for (auto e: choice) std::cout << e << " ";
        std::cout << '\n';

        // construct a vector to stored the received messages.
        AlignedUnVector<block> rMsgs(totalOTs);

        try {
            if (randomOT) {
                // perform  totalOTs random OTs, the results will be written
                // to msgs.
                cp::sync_wait(receiver.receive(choice, rMsgs, prng, chl));
            } else {
                // perform  totalOTs chosen message OTs, the results will be
                // written to msgs.
                cp::sync_wait(
                    receiver.receiveChosen(choice, rMsgs, prng, chl));
            }

            std::cout << "Results: ";
            for (auto& e: rMsgs) std::cout << e << std::endl;
        } catch (std::exception& e) {
            std::cout << e.what() << std::endl;
            chl.close();
        }
    } else {
        // construct a vector to stored the random send messages.
        AlignedUnVector<std::array<block, 2>> sMsgs(totalOTs);

        // if delta OT is used, then the user can call the following
        // to set the desired XOR difference between the zero messages
        // and the one messages.
        //
        //     senders[i].setDelta(some 128 bit delta);
        //
        try {
            if (randomOT) {
                // perform the OTs and write the random OTs to msgs.
                cp::sync_wait(sender.send(sMsgs, prng, chl));
            } else {
                // Populate msgs with something useful...
                prng.get(sMsgs.data(), sMsgs.size());

                sMsgs[0][0] = block(1919810), sMsgs[0][1] = block(114514); // explicit constructor
                for (auto& e: sMsgs) {
                    std::cout << "bruh " << e[0] << " ; " << e[1] << "\n";
                }

                // perform the OTs. The receiver will learn one
                // of the messages stored in msgs.
                cp::sync_wait(sender.sendChosen(sMsgs, prng, chl));
            }
        } catch (std::exception& e) {
            std::cout << e.what() << std::endl;
            chl.close();
        }
    

        // make sure all messages have been sent.
        cp::sync_wait(chl.flush());
    }

    auto e = timer.setTimePoint("finish");
    auto milli =
        std::chrono::duration_cast<std::chrono::milliseconds>(e - s).count();

    auto com = 0;  // (chls[0].getTotalDataRecv() + chls[0].getTotalDataSent())*
                   // numThreads;

    if (role == Role::Sender)
        lout << tag << " n=" << Color::Green << totalOTs << " " << milli
             << " ms  " << com << " bytes" << std::endl
             << Color::Default;

    if (isVerbose && role == Role::Sender) {  // HACK remove
        if (role == Role::Sender)
            lout << " **** sender ****\n" << sendTimer << std::endl;

        if (role == Role::Receiver)
            lout << " **** receiver ****\n" << recvTimer << std::endl;
    }
    cp::sync_wait(chl.flush());
}

#endif