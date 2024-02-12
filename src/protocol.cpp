#include "protocol.tpp"
using namespace osuCrypto;
// HACK enable iknp
void noHash(IknpOtExtSender& s, IknpOtExtReceiver& r) {
    s.mHash = false;
    r.mHash = false;
}
