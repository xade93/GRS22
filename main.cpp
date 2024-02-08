#include <bits/stdc++.h>
#include "protocol.tpp"
#include "okvs.cpp"

int main() {
    auto randomSource = std::make_unique<std::random_device>();
    okvs::RandomBooleanPaXoS<2,2,2,10> pxo{std::move(randomSource)};
    std::vector<std::pair<std::bitset<2>, std::bitset<2>>> kvs;
    std::bitset<2> k(3), v(2);
    kvs.emplace_back(k, v);
    auto ret = pxo.encode(kvs);

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