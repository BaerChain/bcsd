//
// Created by jestjest on 11/27/17.
//

#include <utility>
#include "node/kademlianode.hpp"


namespace kdml {

    KademliaNode::KademliaNode(std::string ipAddr, unsigned short port)
            : protocol({ipAddr, port}) {}

    void KademliaNode::bootstrap(const kdml::NodeInfo& peer) {
        protocol.bootstrap(peer);
    }

    void KademliaNode::join() {
        protocol.join();
    }

    void KademliaNode::get(std::string key, kdml::GetCallback callback) {

        namespace mp = boost::multiprecision;

        std::vector<unsigned char> hash(32);
        picosha2::hash256(key.begin(), key.end(), hash.begin(), hash.end());

        mp::uint256_t keyHash;
        mp::import_bits(keyHash, hash.begin(), hash.end());

        protocol.async_get(keyHash, std::move(callback));

    }

    void KademliaNode::put(std::string key) {
        namespace mp = boost::multiprecision;

        std::vector<unsigned char> hash(32);
        picosha2::hash256(key.begin(), key.end(), hash.begin(), hash.end());

        mp::uint256_t keyHash;
        mp::import_bits(keyHash, hash.begin(), hash.end());

        protocol.async_store(keyHash, protocol.getOwner());
    }
}
