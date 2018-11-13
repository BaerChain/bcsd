//
// Created by jestjest on 12/8/17.
//

#ifndef SIMPLE_KADEMLIA_NETWORK_HPP
#define SIMPLE_KADEMLIA_NETWORK_HPP

#include <map>
#include "callbacks.hpp"
#include <boost/asio/streambuf.hpp>
#include "node/nodeinfo.hpp"
#include "requests/request.hpp"
#include "routing tree/routingtree.hpp"


namespace kdml {

    class Protocol;

    namespace net {

        class Network {

            NodeInfo owner;
            uint32_t next_tid;
            //map tids to requests to lookup callbacks later
            std::map<uint32_t, Request> requests;

            Protocol *protocol;
            boost::asio::ip::udp::socket socket;
            boost::asio::ip::udp::endpoint remote;

            void send_RPC(NodeInfo& dest, std::shared_ptr<net::Message> message,
                          Callback onSend = nullptr);

        public:

            explicit Network(NodeInfo node, boost::asio::io_service &ioService, Protocol *protocol);
            void startReceive();
            boost::system::error_code populateBuf(boost::asio::streambuf &sb);
            NodeInfo getRemotePeer();
            void handleSend(NodeInfo dest, std::shared_ptr<Message> msg,
                            Callback onSend,
                            const boost::system::error_code& error,
                            std::size_t /*bytes_transferred*/);

            bool containsRequest(uint32_t tid);
            void removeRequest(uint32_t tid);
            Request getRequest(uint32_t tid);

            void send_ping(NodeInfo& dest, SimpleCallback onPong);
            void send_find_node(mp::uint256_t key, NodeInfo& dest);
            void send_find_value(mp::uint256_t key, NodeInfo& dest);
            void send_store(mp::uint256_t key, NodeInfo& dest);

            void send_ping_response(NodeInfo& dest, uint32_t tid);
            void send_find_node_response(NodeInfo& dest, Nodes nodes, uint32_t tid);
            void send_find_value_response(NodeInfo& dest, bool success, Nodes result, uint32_t tid);
            void send_store_response(NodeInfo& dest, bool success, uint32_t tid);

        };
    }
}

#endif //SIMPLE_KADEMLIA_NETWORK_HPP