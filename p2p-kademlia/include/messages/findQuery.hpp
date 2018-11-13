//
// Created by jestjest on 11/19/17.
//

#ifndef SIMPLE_KADEMLIA_FINDQUERY_HPP
#define SIMPLE_KADEMLIA_FINDQUERY_HPP

#include "queryMessage.hpp"
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>

namespace kdml {
    namespace net {

        class FindQuery : public QueryMessage {
        protected:
            mp::uint256_t targetId{};
        public:
            FindQuery(mp::uint256_t id, uint32_t tid, mp::uint256_t target,
                      QueryType qtype)
                    : QueryMessage(std::move(id), tid, qtype),
                      targetId(std::move(target)) {
                assert(qtype == QueryType::FIND_VALUE || qtype == QueryType::FIND_NODE);
            }

            void print(std::ostream& os) const override {
                os << "[" << mtype << "][" << qtype << "][TID="
                   << tid << "][queryingID=" << id << "][target="
                   << targetId << "]";
            }

            mp::uint256_t getTarget() { return targetId; }

            template<class Archive>
            void save(Archive& ar) const {
                std::vector<unsigned char> targetVec(32);
                mp::export_bits(targetId, std::back_inserter(targetVec), 8);
                ar(cereal::base_class<QueryMessage>(this), targetVec);
            }

            template<class Archive>
            void load(Archive& ar) {
                std::vector<unsigned char> targetVec(32);
                ar(cereal::base_class<QueryMessage>(this), targetVec);
                mp::import_bits(targetId, targetVec.begin(), targetVec.end());
            }

            template<class Archive>
            static void load_and_construct(Archive& ar, cereal::construct<FindQuery>& construct) {
                uint32_t tid{};
                MessageType mtype{};
                QueryType qtype{};
                std::vector<unsigned char> idVec(32);
                std::vector<unsigned char> targetVec(32);
                mp::uint256_t id;
                mp::uint256_t target;

                ar(tid, mtype, qtype, idVec, targetVec);
                mp::import_bits(id, idVec.begin(), idVec.end());
                mp::import_bits(target, targetVec.begin(), targetVec.end());
                construct(id, tid, target, qtype);
            }
        };
    }
}

namespace cereal {
    template <class Archive>
    struct specialize<Archive, kdml::net::FindQuery, cereal::specialization::member_load_save> {};
    // cereal no longer has any ambiguity when serializing
}

CEREAL_REGISTER_TYPE(kdml::net::FindQuery);

#endif //SIMPLE_KADEMLIA_FINDQUERY_HPP
