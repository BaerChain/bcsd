//
// Created by jestjest on 11/30/17.
//

#ifndef SIMPLE_KADEMLIA_CALLBACKS_HPP
#define SIMPLE_KADEMLIA_CALLBACKS_HPP

#include <node/nodeinfo.hpp>
#include <messages/message.hpp>
#include <messages/responseMessage.hpp>

namespace kdml {
    namespace mp = boost::multiprecision;
    using Nodes = std::vector<NodeInfo>;

    using Callback = std::function<void()>;
    using SimpleCallback = std::function<void(bool)>;

    using GetCallback = std::function<void(Nodes)>;
    using FindNodeCallback = std::function<void(Nodes, mp::uint256_t)>;
    using FindValueCallback = std::function<void(Nodes, mp::uint256_t, bool, GetCallback)>;

    using completeReqCallback = std::function<void(std::shared_ptr<net::ResponseMessage>, bool)>;
}

#endif //SIMPLE_KADEMLIA_CALLBACKS_HPP
