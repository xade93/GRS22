#include <bits/stdc++.h>
#include "protocol.tpp"

int main() {
    int n = 0;  // exponent of OT instance count, default to 20 when n == 0.
    int t = 1;  // numThreads
    std::string ip = "localhost:1212";

    auto thrd = std::thread([&] {
        try {
            TwoChooseOneProtocol<IknpOtExtSender, IknpOtExtReceiver>(
                Role::Sender, n, t, ip, "iknp");
        }  // tag is not impt
        catch (std::exception& e) {
            lout << e.what() << std::endl;
        }
    });

    try {
        TwoChooseOneProtocol<IknpOtExtSender, IknpOtExtReceiver>(
            Role::Receiver, n, t, ip, "iknp");
    } catch (std::exception& e) {
        lout << e.what() << std::endl;
    }
    thrd.join();

    return 0;
}