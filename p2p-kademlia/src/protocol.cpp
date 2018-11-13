//
// Created by jestjest on 11/19/17.
//

#include <boost/bind.hpp>
#include <cereal/archives/binary.hpp>
#include <messages/queryMessage.hpp>
#include <messages/responseMessage.hpp>
#include <messages/findQuery.hpp>
#include <messages/storeQuery.hpp>
#include <queue>
#include <messages/findValueResponse.hpp>
#include <messages/findNodeResponse.hpp>
#include "protocol.hpp"
#include "network.hpp"

#include <unistd.h>



namespace kdml {

    using boost::asio::ip::udp;
    namespace asio = boost::asio;
    namespace mp = boost::multiprecision;

    Protocol::Protocol(const NodeInfo& node)
            : owner(node),
              routingTable(node.id),
              ioService{},
              ioLock(new asio::io_service::work(ioService)) {

        network = new net::Network(node, ioService, this);
        network->startReceive();
        ioThread = std::thread([this]() { ioService.run(); });
		
    }

    void Protocol::async_get(mp::uint256_t key, kdml::GetCallback callback) {
        node_lookup(key, callback, true, false);
    }

    void Protocol::async_store(mp::uint256_t key, NodeInfo value) {
        node_lookup(key, NULL, false, true);
    }

    // TODO queue user commands while bootstrapping).
    void Protocol::bootstrap(const NodeInfo& peer) {
        Nodes endpoints = resolveEndpoint(peer);
        probePeers(std::move(endpoints));
    }

    void Protocol::join() {
		//ioService.run();
		ioService.stop();
		if (ioThread.joinable()) {
			std::cout << "ioTheard starting..." << std::endl;
			ioThread.join();
			std::cout << "ioTheard end ..." << std::endl;
		}
		else
		{
			std::cout << "ioTheard already started..." << std::endl;
		}
		//sleep(10);
    }

    void Protocol::handleReceive(const boost::system::error_code& error,
                                 std::size_t /*bytes_transferred*/) {

        if (error == asio::error::operation_aborted) {
            return;
        }

        asio::streambuf sb;
        boost::system::error_code recvError = network->populateBuf(sb);

        if (!recvError) {
            NodeInfo sender(network->getRemotePeer());
            std::shared_ptr<kdml::net::Message> message;
            {
                std::istream istream(&sb);
                cereal::BinaryInputArchive iarchive(istream);
                iarchive(message);
            }
            std::cout << "Parsed message: " << *message << std::endl;
            handleMessage(message, sender);
        }
        network->startReceive();
    }

    void Protocol::probePeers(Nodes endpoints) {
        if (endpoints.empty()) {
            throw std::string("Failed to find peers");
        } else {
            NodeInfo ep = endpoints.back();
            endpoints.pop_back();

            SimpleCallback onPong = [=](bool failure) {
                if (failure) {
                    probePeers(endpoints);
                } else {
                    auto bucket = routingTable.insertNode(ep);
                    node_lookup(owner.id, [bucket](Nodes found) {
                        // TODO: refresh buckets after bucket
                    }, false, false);
                }
            };

            network->send_ping(ep, onPong);
        }
    }

    Nodes Protocol::resolveEndpoint(const NodeInfo& endpoint) {
        udp::resolver resolver(ioService);
        udp::resolver::query query(udp::v4(), endpoint.getIpAddr(),
                                   std::to_string(endpoint.port));
        Nodes eps;
        auto resolvedEndpoints = resolver.resolve(query);
        decltype(resolvedEndpoints) end;

        while (resolvedEndpoints != end) {
            auto ep = (*resolvedEndpoints).endpoint();
            eps.emplace_back(NodeInfo{ep.address().to_string(), ep.port()});
            resolvedEndpoints++;
        }
        return eps;
    }

    void Protocol::handleMessage(std::shared_ptr<net::Message> msg,
                                 NodeInfo sender) {

        switch (msg->getMessageType()) {
            case net::MessageType::ERROR: {
                std::cerr << "Received error message: " << *msg << std::endl;
                break;
            }
            case net::MessageType::QUERY: {
                routingTable.insertNode(sender);
                handleQuery(std::dynamic_pointer_cast<net::QueryMessage>(msg), std::move(sender));
                break;
            }
            case net::MessageType::RESPONSE: {
                routingTable.insertNode(sender);
                handleResponse(std::dynamic_pointer_cast<net::ResponseMessage>(msg));
                break;
            }
        }
    }

    void Protocol::find_value_callback(Nodes nodes, bool found, GetCallback callback) {
        if(found) {
            std::cout << "Found nodes: first one: " << (*nodes.begin()).getIpAddr() << std::endl;
            callback(nodes);
        } else {
            std::cout << "Did not find a knowledgeable node." << std::endl;
            callback({});
        }
    }

    void Protocol::store_callback(mp::uint256_t key, Nodes nodes) {
        for(NodeInfo node : nodes) {
            network->send_store(key, node);
        }
    }

    /*
     * Pick a closest nodes to key from bucket, send asynchronous FIND_NODE rpcs to each.
     */
    void Protocol::node_lookup(mp::uint256_t key, GetCallback callback, bool findValue, bool store) {

        std::vector<NodeInfo> a_closest_nodes = routingTable.getAClosestNodes(ALPHA, key);
        RequestState request_state;
        request_state.key = key;
        request_state.findValue = findValue;
        request_state.store = store;
        request_state.callback = callback;
        for(NodeInfo node : a_closest_nodes) {
            NodeInfoWrapper node_wrapper(key, node);
            request_state.k_closest_nodes.insert(node_wrapper);
            request_state.responses_waiting++;
            if (request_state.findValue) {
                network->send_find_value(key, node);
            } else {
                network->send_find_node(key, node);
            }
        }
        lookups[key] = request_state;
    }

    //TODO: Nodes that fail to respond should be removed from consideration unless they respond

    void Protocol::node_lookup_callback(mp::uint256_t sender, RequestState& request_state, Nodes k_nodes,
                                        mp::uint256_t key, bool found) {

        request_state.responses_waiting--;
        request_state.responded_nodes.insert(sender);

        if (found) {
            assert(request_state.findValue);
            // If found, k_nodes will be the list of values associated with key
            find_value_callback(k_nodes, found, request_state.callback);
            lookups.erase(lookups.find(key), lookups.end());
            return;
        }

        for(NodeInfo node : k_nodes) {
            NodeInfoWrapper node_wrapper(key, node);
            request_state.k_closest_nodes.insert(node_wrapper);
        }

        // Query alpha more nodes
        // Terminates when has queried and heard responses from k closest nodes
        int responded = 0;
        bool k_responded = true;
        for (auto it=request_state.k_closest_nodes.begin(); it!=request_state.k_closest_nodes.end(); ++it) {
            if (request_state.responses_waiting >= ALPHA) break;
            if (k_responded && responded >= kdml::K) break;

            NodeInfoWrapper node_wrapper = *it;

            // Keep track if k closest nodes have responded
            auto responded_it = request_state.responded_nodes.find(node_wrapper.node.id);
            if (responded_it != request_state.responded_nodes.end()) {
                responded++;
            } else {
                k_responded = false;
            }

            // Do not query if node has already been queried
            auto queried_it = request_state.queried_nodes.find(node_wrapper.node.id);
            if (queried_it != request_state.queried_nodes.end()) {
                continue;
            }

            request_state.responses_waiting++;
            request_state.queried_nodes.insert(node_wrapper.node.id);
            if (request_state.findValue) {
                network->send_find_value(key, node_wrapper.node);
            } else {
                network->send_find_node(key, node_wrapper.node);
            }
        }

        if (request_state.responses_waiting == 0) {
            Nodes k_closest_nodes_list;
            for (auto it=request_state.k_closest_nodes.begin(); it!=request_state.k_closest_nodes.end()
                    && k_closest_nodes_list.size() < kdml::K; ++it) {
                NodeInfoWrapper node_wrapper = *it;
                k_closest_nodes_list.push_back(node_wrapper.node);
            }
            if (request_state.findValue) {
                find_value_callback(k_closest_nodes_list, found, request_state.callback);
            } else if (request_state.store){
                store_callback(key, k_closest_nodes_list);
            } else {
                std::cout << "Completed lookup for bootstrapping" << std::endl;
            }

            lookups.erase (lookups.find(key), lookups.end());
        }
    }


    void Protocol::handleQuery(std::shared_ptr<net::QueryMessage> msg,
                               NodeInfo sender) {

        switch (msg->getQueryType()) {
            case net::QueryType::PING: {
                network->send_ping_response(sender, msg->getTid());
                break;
            }
            case net::QueryType::FIND_NODE: {
                auto query = std::dynamic_pointer_cast<net::FindQuery>(msg);
                auto nodes = routingTable.getKClosestNodes(query->getTarget());
                network->send_find_node_response(sender, nodes, msg->getTid());
                break;
            }
            case net::QueryType::FIND_VALUE: {
                auto query = std::dynamic_pointer_cast<net::FindQuery>(msg);
                auto value = storage.find(query->getTarget());
                bool found = (value != storage.end());
                Nodes result;
                if (found) {
                    result = value->second;
                } else {
                    result = routingTable.getKClosestNodes(query->getTarget());
                }
                network->send_find_value_response(sender, found, result,
                                                  msg->getTid());
                break;
            }
            case net::QueryType::STORE: {
                auto query = std::dynamic_pointer_cast<net::StoreQuery>(msg);
                bool success = false;

                try {
                    storage[query->getKey()].emplace_back(query->getVal());
                    success = true;
                } catch (...) {}

                network->send_store_response(sender, success, msg->getTid());
                break;
            }
        }
    }

    void Protocol::handleResponse(std::shared_ptr<net::ResponseMessage> msg) {
        uint32_t tid = msg->getTid();

        if (!network->containsRequest(tid)) {
            std::cerr << "Received alien message" << *msg << std::endl;
            return;
        }

        Request outstanding = network->getRequest(tid);
        if (msg->getQueryType() == net::QueryType::PING) {
            network->removeRequest(tid);
            outstanding.onDone(msg, false);
        } else {
            auto it = lookups.find(outstanding.key);
            if (it != lookups.end()) {
                RequestState& request_state = it->second;
                if (outstanding.findValue) {
                    auto value_res = std::dynamic_pointer_cast<net::FindValueResponse>(msg);
                    node_lookup_callback(msg->id, request_state, value_res->data, outstanding.key, value_res->found);
                } else {
                    auto find_res = std::dynamic_pointer_cast<net::FindNodeResponse>(msg);
                    node_lookup_callback(msg->id, request_state, find_res->nodes, outstanding.key, false);
                }
            } else {
                std::cout << "Got response for nonexistent nodes lookup" << std::endl;
            }
        }
    }

}
