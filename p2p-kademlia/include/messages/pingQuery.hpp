//
// Created by jestjest on 11/19/17.
//

#ifndef SIMPLE_KADEMLIA_PINGQUERY_HPP
#define SIMPLE_KADEMLIA_PINGQUERY_HPP


#include "queryMessage.hpp"
#include "cereal/archives/binary.hpp"

namespace kdml {
    namespace net {

        class PingQuery : public QueryMessage {
        public:
            PingQuery(mp::uint256_t id, uint32_t tid)
                    : QueryMessage(std::move(id), tid, QueryType::PING) {}

            void print(std::ostream& os) const override {
                os << "[" << mtype << "][" << qtype << "][TID="
                   << tid << "][nodeID=" << id << "]";
            }

            template<class Archive>
            void serialize(Archive& ar) {
                ar(cereal::base_class<QueryMessage>(this));
            }

            template<class Archive>
            static void load_and_construct(Archive& ar, cereal::construct<PingQuery>& construct) {
                uint32_t tid{};
                MessageType mtype{};
                QueryType qtype{};
                std::vector<unsigned char> idVec(32);
                mp::uint256_t id;

                ar(tid, mtype, qtype, idVec);
                mp::import_bits(id, idVec.begin(), idVec.end());
                construct(id, tid);
            }
        };
    }
}

namespace cereal {
    template <class Archive>
    struct specialize<Archive, kdml::net::PingQuery, cereal::specialization::member_serialize> {};
    // cereal no longer has any ambiguity when serializing
}

CEREAL_REGISTER_TYPE(kdml::net::PingQuery);

#endif //SIMPLE_KADEMLIA_PINGQUERY_HPP
