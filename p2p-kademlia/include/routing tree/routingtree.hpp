/*
 * Created by sierrakn on 11/11/17.
 * Routing Tree contains (up to) one k-bucket for each bit of node id.
 * Each k-bucket has up to k node infos and covers section of node id space with some common prefix
 * Starts with single k-bucket covering whole id space,
 * when learns of node u if bucket is not full, just inserts it into bucket
 * If bucket is full and contains node's own id split it. If not drop new node =>
 * If node's id starts with 1, only one k-bucket devoted to id's starting with 0 (distance further)
 * After splitting contents reattempt insertion
 */

#ifndef SIMPLE_KADEMLIA_ROUTINGTABLE_HPP
#define SIMPLE_KADEMLIA_ROUTINGTABLE_HPP

#include "kbucket.hpp"


namespace kdml {

    namespace mp = boost::multiprecision;

    struct RoutingTreeNode {
        kBucket *bucket;
        RoutingTreeNode *zero;
        RoutingTreeNode *one;
        RoutingTreeNode *parent; //todo: delete parent?
        bool isLeaf; //if bucket != NULL

        RoutingTreeNode(kBucket *bucket, RoutingTreeNode *parent) {
            this->bucket = bucket;
            this->parent = parent;
            this->zero = NULL;
            this->one = NULL;
            this->isLeaf = true;
        };
    };

    class RoutingTree {

        RoutingTreeNode *root;
        mp::uint256_t m_node_id;
        int num_contacts;

        RoutingTreeNode *getTreeNode(RoutingTreeNode *treeNode, mp::uint256_t node_id, uint16_t bit_index) {
            if (treeNode -> isLeaf) return treeNode;
            mp::uint256_t branch = node_id & (1 << bit_index);
            if (branch) {
                return getTreeNode(treeNode->one, node_id, bit_index-1);
            }
            return getTreeNode(treeNode->zero, node_id, bit_index-1);
        }

        RoutingTreeNode *getTreeNode(mp::uint256_t node_id) {
            return getTreeNode(root, node_id, 255);
        }

        int getClosestNodes(std::vector<NodeInfo>& nodes, int desired_num, int num, mp::uint256_t key,
                             RoutingTreeNode *treeNode, uint16_t bit_index) {

            if (num == desired_num) return num;
            if (treeNode -> isLeaf) {
                kBucket *bucket = treeNode->bucket;
                assert(bucket != NULL);
                num += bucket->getNodes(nodes, desired_num-num);
                return num;
            }

            // First get values from correct branch, if not enough get next closest values from other branch
            RoutingTreeNode *first = treeNode->zero;
            RoutingTreeNode *second = treeNode->one;
            mp::uint256_t branch = key & (1 << bit_index);
            if (branch) {
                first = treeNode->one;
                second = treeNode->zero;
            }

            num += getClosestNodes(nodes, desired_num, num, key, first, bit_index-1);
            num += getClosestNodes(nodes, desired_num, num, key, second, bit_index-1);

            return num;
        }

        std::vector<NodeInfo> getClosestNodes(int num, mp::uint256_t key) {
            std::vector<NodeInfo> closest;
            getClosestNodes(closest, num, 0, key, root, 255);
            return closest;
        }

    public:

        RoutingTree(mp::uint256_t node_id) {
            num_contacts = 0;
            m_node_id = node_id;
            root = new RoutingTreeNode(new kBucket(0, 0), NULL);
        }

        // Return k nodes from tree node, and if not enough keep getting more from parent
        // Unless < k in whole tree in which case returns all nodes it knows about
        std::vector<NodeInfo> getKClosestNodes(mp::uint256_t key) {
            //todo: make k in kbucket global config?
            return getClosestNodes(kdml::K, key);
        }

        std::vector<NodeInfo> getAClosestNodes(int a, mp::uint256_t key) {
            std::cout << num_contacts << std::endl;
            return getClosestNodes(a, key);
        }

        bool insertNode(NodeInfo node) {
            std::cout << "Inserting node " << node.getIpAddr() << ":" << node.port << std::endl;
            mp::uint256_t node_id = node.id;
            RoutingTreeNode *treeNode = getTreeNode(node_id);
            kBucket *bucket = treeNode->bucket;
            bool success = bucket->insertNode(node);

            // If bucket is full and its range contains our node id, split it
            while (!success && bucket->rangeContainsId(m_node_id)) {
                kBucket *newBucket = bucket->split();

                // original bucket contains zero branch, new bucket now contains one branch
                RoutingTreeNode *zeroTreeNode = new RoutingTreeNode(bucket, treeNode);
                RoutingTreeNode *oneTreeNode = new RoutingTreeNode(newBucket, treeNode);

                treeNode->zero = zeroTreeNode;
                treeNode->one = oneTreeNode;
                treeNode->bucket = NULL;
                treeNode->isLeaf = false;

                // try insertion again
                treeNode = getTreeNode(node_id);
                bucket = treeNode->bucket;
                success = bucket->insertNode(node);
            }
            return success;
        }

    };
}


#endif //SIMPLE_KADEMLIA_ROUTINGTABLE_HPP
