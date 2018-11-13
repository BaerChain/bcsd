#ifndef SIMPLE_KADEMLIA_PROTOCOL_HPP
#define SIMPLE_KADEMLIA_PROTOCOL_HPP

#include "node/nodeinfo.hpp"
#include "routing tree/routingtree.hpp"
#include "callbacks.hpp"
#include <boost/asio.hpp>
#include <thread>
#include <messages/responseMessage.hpp>
#include <messages/queryMessage.hpp>
#include <queue>

namespace kdml {

    namespace net {
        class Network;
    }

    struct NodeInfoWrapper {
        boost::multiprecision::uint256_t key;
        NodeInfo node;

        NodeInfoWrapper(boost::multiprecision::uint256_t key, NodeInfo node) {
            this->key = key;
            this->node = node;
        }
    };

    static bool node_comp (NodeInfoWrapper a, NodeInfoWrapper b) {
        return (a.key ^ a.node.id) < (b.key ^ b.node.id);
    }

    class NodeComparison {
    public:
        bool operator() (NodeInfoWrapper a, NodeInfoWrapper b) {
            return node_comp(a, b);
        }
    };

    struct RequestState {
        boost::multiprecision::uint256_t key;
        GetCallback callback;
        int responses_waiting {0};
        bool findValue {false};
        bool store {false};

        std::set<boost::multiprecision::uint256_t> queried_nodes;
        std::set<boost::multiprecision::uint256_t> responded_nodes;

        std::set<NodeInfoWrapper, NodeComparison> k_closest_nodes;
    };

    class Protocol {

        NodeInfo owner;
        net::Network* network;
        RoutingTree routingTable;

        std::map<boost::multiprecision::uint256_t, Nodes> storage;

        boost::asio::io_service ioService;
        std::unique_ptr<boost::asio::io_service::work> ioLock;

        std::thread ioThread;

        void handleMessage(std::shared_ptr<net::Message> msg, NodeInfo sender);
        void handleQuery(std::shared_ptr<net::QueryMessage> msg, NodeInfo sender);
        void handleResponse(std::shared_ptr<net::ResponseMessage> msg);

        void probePeers(Nodes endpoints);
        Nodes resolveEndpoint(const NodeInfo& ep);

        //map of keys -> request state for lookup procedure
        std::map<boost::multiprecision::uint256_t, RequestState> lookups;

        void node_lookup_callback(mp::uint256_t sender, RequestState& request_state, Nodes k_nodes,
                                            mp::uint256_t key, bool found);

        void find_value_callback(Nodes nodes, bool found, GetCallback callback);
        void store_callback(boost::multiprecision::uint256_t key, Nodes nodes);
	

    public:

        // number of concurrent lookups in node lookup
        static const int ALPHA = 3;

        explicit Protocol(const NodeInfo& owner);

        void async_get(boost::multiprecision::uint256_t key, kdml::GetCallback callback);
        void async_store(boost::multiprecision::uint256_t key, NodeInfo value);

        void bootstrap(const NodeInfo& peer);
        void join();
		void downwork() {}
        NodeInfo getOwner() { return owner; };

        void node_lookup(mp::uint256_t key, GetCallback callback, bool findValue, bool store);
        void handleReceive(const boost::system::error_code& error,
                           std::size_t /*bytes_transferred*/);



    };
}

#endif //SIMPLE_KADEMLIA_PROTOCOL_HPP
