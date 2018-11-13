//
// Created by jestjest on 11/19/17.
//

#ifndef SIMPLE_KADEMLIA_STOREQUERY_HPP
#define SIMPLE_KADEMLIA_STOREQUERY_HPP

#include <cereal/archives/binary.hpp>
#include <node/nodeinfo.hpp>
#include <messages/queryMessage.hpp>

namespace kdml {
    namespace net {

        class StoreQuery : public QueryMessage {

        protected:
            mp::uint256_t key{};
            NodeInfo val{};

        public:
            StoreQuery(mp::uint256_t id, uint32_t tid, mp::uint256_t key, NodeInfo val)
                    : QueryMessage(std::move(id), tid, QueryType::STORE),
                      key(std::move(key)), val(std::move(val)) {}

            void print(std::ostream& os) const override {
                os << "[" << mtype << "][" << qtype << "][TID="
                   << tid << "][queryingID=" << id << "][key="
                   << key << "][val=" << val << "]";
            }

            mp::uint256_t getKey() { return key; }
            NodeInfo getVal() { return val; }

            template<class Archive>
            void save(Archive& ar) const {
                std::vector<unsigned char> keyVec(32);
                mp::export_bits(key, std::back_inserter(keyVec), 8);
                ar(cereal::base_class<QueryMessage>(this), keyVec, val);
            }

            template<class Archive>
            void load(Archive& ar) {
                std::vector<unsigned char> keyVec(32);
                ar(cereal::base_class<QueryMessage>(this), keyVec, val);
                mp::import_bits(key, keyVec.begin(), keyVec.end());
            }

            template<class Archive>
            static void load_and_construct(Archive& ar, cereal::construct<StoreQuery>& construct) {
                uint32_t tid{};
                MessageType mtype{};
                QueryType qtype{};
                std::vector<unsigned char> idVec(32);
                std::vector<unsigned char> keyVec(32);
                mp::uint256_t id;
                mp::uint256_t key;
                NodeInfo val;

                ar(tid, mtype, qtype, idVec, keyVec, val);
                mp::import_bits(id, idVec.begin(), idVec.end());
                mp::import_bits(key, keyVec.begin(), keyVec.end());
                construct(id, tid, key, val);
            }
        };
    }
}

namespace cereal {
    template <class Archive>
    struct specialize<Archive, kdml::net::StoreQuery, cereal::specialization::member_load_save> {};
    // cereal no longer has any ambiguity when serializing
}

CEREAL_REGISTER_TYPE(kdml::net::StoreQuery);

#endif //SIMPLE_KADEMLIA_STOREQUERY_HPP
