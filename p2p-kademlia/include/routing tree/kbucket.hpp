/*
 * Created by sierrakn on 11/11/17.
 * Contains up to k node infos.
 * Kept sorted by last time seen - least recently seen at head, most at tail
 */

#ifndef SIMPLE_KADEMLIA_KBUCKET_HPP
#define SIMPLE_KADEMLIA_KBUCKET_HPP

#include <list>
#include <utility>
#include "node/nodeinfo.hpp"

namespace kdml {

    namespace mp = boost::multiprecision;
    static const int K = 20;

    class kBucket {


        std::list<NodeInfo> contacts;
        mp::uint256_t prefix;
        uint16_t tree_level;

    public:

        kBucket(mp::uint256_t prefix, uint16_t tree_level) {
            this->prefix = std::move(prefix);
            this->tree_level = tree_level;
        }

        bool contains_node(NodeInfo node) {
            auto i = contacts.begin();
            while (i != contacts.end()) {
                NodeInfo other_node = *i;
                if (other_node.id == node.id) {
                    return true;
                }
                i++;
            }
            return false;
        }

        bool insertNode(NodeInfo node) {
            if (contains_node(node)) return true;

            if (contacts.size() < K) {
                contacts.push_back(node);
                return true;
            }
            return false;
        }

        // Checks if first tree_level bits of prefix and node_id are the same
        bool rangeContainsId(const mp::uint256_t &node_id) {
            return ((prefix >> (256 - tree_level)) ^ (node_id >> (256 - tree_level))) == 0;
        }

        // Iterates over node info list and
        void splitNodes(kBucket *new_bucket) {
            auto i = contacts.begin();
            while (i != contacts.end()) {
                NodeInfo node = *i;
                if (new_bucket->rangeContainsId(node.id)) {
                    new_bucket->insertNode(node);
                    i = contacts.erase(i);
                } else {
                    i++;
                }
            }
        }

        // Splits bucket and returns one branch bucket.
        // Splits contents between new buckets
        // todo: check if tree level is max (probably won't happen)
        kBucket *split() {
            tree_level++;
            mp::uint256_t new_prefix = prefix | (1 << (256 - tree_level));
            auto *new_bucket = new kBucket(new_prefix, tree_level);
            splitNodes(new_bucket);
            return new_bucket;
        }

        // adds num_nodes to vector or if has less than num_nodes,
        // adds all contacts to vector.
        // Returns how many nodes added.
        int getNodes(std::vector<NodeInfo>& nodes, int num_nodes) {
            int num_added = 0;
            auto i = contacts.begin();
            for (; i != contacts.end() && num_added < num_nodes; i++) {
                NodeInfo node = *i;
                nodes.push_back(node);
                num_added++;
            }
            return num_added;
        }
    };
}

#endif //SIMPLE_KADEMLIA_KBUCKET_HPP
