//
// Created by jestjest on 12/8/17.
//

#ifndef SIMPLE_KADEMLIA_REQUEST_HPP
#define SIMPLE_KADEMLIA_REQUEST_HPP

namespace kdml {

    namespace mp = boost::multiprecision;

    struct Request {

        mp::uint256_t key{};
        bool findValue{false};
        completeReqCallback onDone{};

        explicit Request(completeReqCallback onDone) : onDone(onDone) {}
        Request(mp::uint256_t key, bool findValue, completeReqCallback onDone)
                : key(key), findValue(findValue), onDone(onDone) {}
    };
}

#endif //SIMPLE_KADEMLIA_REQUEST_HPP
