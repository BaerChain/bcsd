//
// Created by jestjest on 12/8/17.
//
#include <iostream>
#include <messages/findQuery.hpp>
#include <messages/pingQuery.hpp>
#include <messages/storeQuery.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/bind.hpp>
#include <messages/pingResponse.hpp>
#include <messages/findNodeResponse.hpp>
#include <messages/storeResponse.hpp>
#include <messages/findValueResponse.hpp>
#include <utility>
#include "network.hpp"
#include "protocol.hpp"

namespace kdml {
    namespace net {

        using boost::asio::ip::udp;
        namespace asio = boost::asio;

        Network::Network(NodeInfo node, asio::io_service &ioService, Protocol *protocol)
                : owner(std::move(node)), next_tid(0), requests{}, protocol(protocol),
                  socket(ioService, udp::endpoint(udp::v4(), node.port)) {}

        void Network::startReceive() {
			//std::cout << "start recive message.." << std::endl;
			//udp::endpoint m_endpointRemote; //收到数据时的端点信息
		    socket.async_receive(
					asio::null_buffers(),
					boost::bind(&kdml::Protocol::handleReceive, protocol,
								asio::placeholders::error,
								asio::placeholders::bytes_transferred));
        }

        boost::system::error_code Network::populateBuf(asio::streambuf &sb) {
            size_t available = socket.available();
            auto buffer = sb.prepare(available);
            boost::system::error_code recvError;
            size_t bytesRead = socket.receive_from(buffer, remote, 0, recvError);
            sb.commit(bytesRead);

            if (recvError) {
                std::cout << "Error: " << recvError.message() << std::endl;
            }
            return recvError;
        }

        NodeInfo Network::getRemotePeer() {
            return {remote.address().to_string(), remote.port()};
        }

        bool Network::containsRequest(uint32_t tid) {
            return requests.find(tid) != requests.end();
        }

        Request Network::getRequest(uint32_t tid) {
            return requests.at(tid);
        }

        void Network::removeRequest(uint32_t tid) {
            requests.erase(tid);
        }

        void Network::send_ping_response(NodeInfo& dest, uint32_t tid) {
            std::shared_ptr<Message> pong = std::make_shared<PingResponse>(owner.id, tid);
            send_RPC(dest, pong);
        }

        void Network::send_find_node_response(NodeInfo& dest, Nodes nodes, uint32_t tid) {
            std::shared_ptr<Message> findNodeRes = std::make_shared<FindNodeResponse>(owner.id, tid, nodes);
            send_RPC(dest, findNodeRes);
        }

        void Network::send_store_response(NodeInfo& dest, bool success, uint32_t tid) {
            std::shared_ptr<Message> storeRes = std::make_shared<StoreResponse>(owner.id, tid, success);
            send_RPC(dest, storeRes);
        }

        void Network::send_find_value_response(NodeInfo& dest, bool success, Nodes result, uint32_t tid) {
            std::shared_ptr<Message> valueRes = std::make_shared<FindValueResponse>(owner.id, tid, success, result);
            send_RPC(dest, valueRes);
        }


        void Network::send_ping(NodeInfo& dest, SimpleCallback onPong) {
            long tid = next_tid++;
            std::shared_ptr<net::Message> ping;
            ping = std::make_shared<net::PingQuery>(owner.id, tid);

            auto onDone = [onPong](std::shared_ptr<net::ResponseMessage>, bool failure) {
                onPong(failure);
            };
            Request req(onDone);

            send_RPC(dest, ping, [this, tid, req]() {
                requests.insert(std::make_pair(tid, req));
            });
        }

        void Network::send_find_node(mp::uint256_t key, NodeInfo& dest) {
            long tid = next_tid++;
            std::shared_ptr<net::Message> find_node;
            find_node = std::make_shared<net::FindQuery>(owner.id, tid, key,
                                                        QueryType::FIND_NODE);

            auto onDone = [](std::shared_ptr<net::ResponseMessage>, bool failure) {
                // now handled in protocol.cpp
            };
            Request req(key, false, onDone);

            send_RPC(dest, find_node, [this, tid, req]() {
                requests.insert(std::make_pair(tid, req));
            });
        }

        void Network::send_find_value(mp::uint256_t key, NodeInfo& dest) {
            long tid = next_tid++;
            std::shared_ptr<net::Message> find_value;
            find_value = std::make_shared<net::FindQuery>(owner.id, tid, key,
                                                          QueryType::FIND_VALUE);

            auto onDone = [](std::shared_ptr<net::ResponseMessage>, bool failure) {
                // now handled in protocol.cpp
            };
            Request req(key, true, onDone);

            send_RPC(dest, find_value, [this, tid, req]() {
                requests.insert(std::make_pair(tid, req));
            });
        }

        void Network::send_store(mp::uint256_t key, NodeInfo& dest) {
            long tid = next_tid++;
            std::shared_ptr<net::Message> store;
            store = std::make_shared<net::StoreQuery>(owner.id, tid, key, owner);

            auto onDone = [](std::shared_ptr<net::ResponseMessage>, bool failure) {
                // now handled in protocol.cpp
            };
            Request req(key, false, onDone);

            send_RPC(dest, store, [this, tid, req]() {
                requests.insert(std::make_pair(tid, req));
            });
        }

        void Network::send_RPC(NodeInfo& dest,
                               std::shared_ptr<net::Message> message,
                               Callback onSend) {

            boost::asio::ip::udp::endpoint destination(
                    boost::asio::ip::address::from_string(dest.getIpAddr()), dest.port);

            boost::asio::streambuf sb;
            {
                std::ostream os(&sb);
                cereal::BinaryOutputArchive oarchive(os);
                oarchive(message);
            }

            socket.async_send_to(sb.data(), destination,
                                 boost::bind(&Network::handleSend, this,
                                             dest, message, std::move(onSend),
                                             boost::asio::placeholders::error,
                                             boost::asio::placeholders::bytes_transferred));
        }

        void Network::handleSend(NodeInfo dest, std::shared_ptr<Message> msg,
                                 Callback onSend,
                                 const boost::system::error_code& error, std::size_t) {
            if (error) {
                std::cerr << "Error sending message: " << *msg << std::endl;
            }

            std::cout << "SEND -> " << dest << " " << *msg << std::endl;
            if (msg->getMessageType() == MessageType::QUERY && onSend){
                onSend();
            }
        }
    }
}