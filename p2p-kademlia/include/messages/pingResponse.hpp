//
// Created by jestjest on 11/19/17.
//

#ifndef SIMPLE_KADEMLIA_PINGRESPONSE_HPP
#define SIMPLE_KADEMLIA_PINGRESPONSE_HPP

#include "responseMessage.hpp"
#include <cereal/archives/binary.hpp>

namespace kdml {
    namespace net {

        class PingResponse : public ResponseMessage {

        public:
            PingResponse(mp::uint256_t id, uint32_t tid)
                    : ResponseMessage(std::move(id), tid, QueryType::PING) {}

            void print(std::ostream& os) const override {
                os << "[" << mtype << "][" << qtype << "][TID="
                   << tid << "][nodeID=" << id << "]";
            }

            template<class Archive>
            void serialize(Archive& ar) {
                ar(cereal::base_class<ResponseMessage>(this));
            }

            template<class Archive>
            static void load_and_construct(Archive& ar, cereal::construct<PingResponse>& construct) {
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
    struct specialize<Archive, kdml::net::PingResponse, cereal::specialization::member_serialize> {};
    // cereal no longer has any ambiguity when serializing
}


CEREAL_REGISTER_TYPE(kdml::net::PingResponse);

#endif //SIMPLE_KADEMLIA_PINGMESSAGE_HPP
