#include <graphene/api/witness_api_object.hpp>

namespace graphene { namespace api {
    witness_api_object::witness_api_object(const witness_object &w, const database& db)
        : id(w.id), owner(w.owner), created(w.created),
          url(to_string(w.url)), total_missed(w.total_missed), last_aslot(w.last_aslot),
          last_confirmed_block_num(w.last_confirmed_block_num),
          signing_key(w.signing_key), props(w.props, db), votes(w.votes),
          virtual_last_update(w.virtual_last_update), virtual_position(w.virtual_position),
          virtual_scheduled_time(w.virtual_scheduled_time), last_work(w.last_work),
          running_version(w.running_version), hardfork_version_vote(w.hardfork_version_vote),
          hardfork_time_vote(w.hardfork_time_vote) {
    }
} } // graphene::api