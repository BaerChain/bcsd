//
// Created by jestjest on 11/11/17.
//

#ifndef SIMPLE_KADEMLIA_KADEMLIANODE_HPP
#define SIMPLE_KADEMLIA_KADEMLIANODE_HPP

#include "callbacks.hpp"
#include "protocol.hpp"
#include <boost/asio/io_service.hpp>
#include <boost/thread.hpp>
#include <functional>
#include <vector>
#include <memory>
#include <thread>

namespace kdml {
    class KademliaNode {
        Protocol protocol;

    public:

        KademliaNode(std::string ipAddr, unsigned short port);

        void bootstrap(const NodeInfo& peer);

        // TODO Wrap key in class
        void get(std::string key, GetCallback callback);
//        void put(std::string key, GetCallback callback);

        // value is just serialized node info
        void put(std::string key);

        void join();
    };
}

#endif //SIMPLE_KADEMLIA_KADEMLIANODE_HPP
