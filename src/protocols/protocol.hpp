#pragma once
#include "common.tpp"
#include "bfss/spatial_hash.tpp"
#include "bfss/trivial_bfss.tpp"
#include "oblivious_transfer.tpp"
#include <libOTe/TwoChooseOne/Iknp/IknpOtExtReceiver.h>
#include <libOTe/TwoChooseOne/Iknp/IknpOtExtSender.h>

// for transfer of fingerprints
#include <cryptoTools/Network/Session.h>
#include <cryptoTools/Network/IOService.h>
#include <cryptoTools/Network/Channel.h>

#include <dbg.h>

using std::string;

// This class defines the abstract behavior of GRS22's protocol.
// server is the one holding structure, and also the one receiving final intersection.
// client receives nothing on default.
// @param bitLength     the length in bits of the point axis (e.g. if X, Y <= 256 then bitLength is 8)
// @param Lambda        the number of extra columns for OKVS (see PaXoS paper)
// @param L             the number of instances OT will be performed. Usually Increasing this decrease error probability exponentially.
// @param cellBitLength the length of bits of the cell. Here we restrict the length to be powers of two, so that coding can be a bit easier. In practice radius can be of any value, and not have to be powers of two.
template<int bitLength, int Lambda, int L, int cellBitLength> 
class GRS22_L_infinity_protocol {
public:
    using Point = std::pair<uint64_t, uint64_t>;
    // Set intersection server, holding structure and yielding intersection result. 
    // use Iknp as OT protocol on default.
    // @param centers       the vector of points storing center of Alice's balls.
    // @param clientIP      IP of client. Port is not needed and is setted above.
    // @param radius        radius of Alice's spheres.
    virtual std::set<Point> SetIntersectionServer(const std::vector<Point>& centers, const string& clientIP, uint32_t radius) = 0;

    // Set intersection client, holding unstructued points and yield nothing. 
    // use Iknp as OT protocol on default.
    // @param points        the vector of Bob's points.
    // @param serverIP      IP of server. Port is not needed and is setted above.
    virtual void SetIntersectionClient(const std::vector<Point>& points, const string& serverIP) = 0;
protected:
    // L-infinity membership test. TODO optimize
    bool membership(const std::vector<Point>& centers, uint64_t x, uint64_t y, uint64_t radius) { 
        for (auto [currX, currY]: centers) {
            if (std::max(
                std::abs((int64_t)currX - (int64_t)x), 
                std::abs((int64_t)currY - (int64_t)y)) <= radius) return true;
        }
        return false;
    };
};