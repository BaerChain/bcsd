//
// Created by jestjest on 11/19/17.
//

#ifndef SIMPLE_KADEMLIA_STORERESPONSE_HPP
#define SIMPLE_KADEMLIA_STORERESPONSE_HPP

#include "responseMessage.hpp"
#include <cereal/archives/binary.hpp>

namespace kdml {
    namespace net {

        class StoreResponse : public ResponseMessage {
        protected:
            bool success;

        public:
            StoreResponse(mp::uint256_t id, uint32_t tid, bool success)
                    : ResponseMessage(std::move(id), tid, QueryType::STORE),
                      success(success) {}

            void print(std::ostream& os) const override {
                os << "[" << mtype << "][" << qtype << "][TID="
                   << tid << "][nodeID=" << id << "][success="
                   << success << "]";
            }

            template<class Archive>
            void serialize(Archive& ar) {
                ar(cereal::base_class<ResponseMessage>(this), success);
            }

            template<class Archive>
            static void load_and_construct(Archive& ar, cereal::construct<StoreResponse>& construct) {
                uint32_t tid{};
                MessageType mtype{};
                QueryType qtype{};
                std::vector<unsigned char> idVec(32);
                mp::uint256_t id;
                bool success{};

                ar(tid, mtype, qtype, idVec, success);
                mp::import_bits(id, idVec.begin(), idVec.end());
                construct(id, tid, success);
            }
        };
    }
}

namespace cereal {
    template <class Archive>
    struct specialize<Archive, kdml::net::StoreResponse, cereal::specialization::member_serialize> {};
    // cereal no longer has any ambiguity when serializing
}

CEREAL_REGISTER_TYPE(kdml::net::StoreResponse);

#endif //SIMPLE_KADEMLIA_STOREMESSAGE_HPP
