#ifndef GOLOS_WITNESS_API_OBJ_HPP
#define GOLOS_WITNESS_API_OBJ_HPP

#include <golos/chain/witness_objects.hpp>
#include <golos/chain/steem_object_types.hpp>

namespace golos { namespace plugins { namespace witness_api {

            using namespace golos::chain;
            using namespace golos::protocol;

            struct witness_api_object {
                witness_api_object(const witness_object &w) : id(w.id), owner(w.owner), created(w.created),
                        url(to_string(w.url)), total_missed(w.total_missed), last_aslot(w.last_aslot),
                        last_confirmed_block_num(w.last_confirmed_block_num),
                        signing_key(w.signing_key), props(w.props), votes(w.votes),
                        virtual_last_update(w.virtual_last_update), virtual_position(w.virtual_position),
                        virtual_scheduled_time(w.virtual_scheduled_time), last_work(w.last_work),
                        running_version(w.running_version), hardfork_version_vote(w.hardfork_version_vote),
                        hardfork_time_vote(w.hardfork_time_vote) {
                }

                witness_api_object() {
                }

                witness_object::id_type id;
                account_name_type owner;
                time_point_sec created;
                std::string url;
                uint32_t total_missed;
                uint64_t last_aslot;
                uint64_t last_confirmed_block_num;
                public_key_type signing_key;
                chain_properties props;
                share_type votes;
                fc::uint128_t virtual_last_update;
                fc::uint128_t virtual_position;
                fc::uint128_t virtual_scheduled_time;
                digest_type last_work;
                version running_version;
                hardfork_version hardfork_version_vote;
                time_point_sec hardfork_time_vote;
            };

        }
    }
} // golos::plugins::witness_api


FC_REFLECT((golos::plugins::witness_api::witness_api_object),
           (id)(owner)(created)(url)(votes)(virtual_last_update)(virtual_position)(virtual_scheduled_time)
           (total_missed)(last_aslot)(last_confirmed_block_num)(signing_key)(props)
           (last_work)(running_version)(hardfork_version_vote)(hardfork_time_vote))

#endif //GOLOS_WITNESS_API_OBJ_HPP
