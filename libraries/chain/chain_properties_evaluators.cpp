#include <graphene/chain/chain_evaluator.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/chain_objects.hpp>

namespace graphene { namespace chain {

    void witness_update_evaluator::do_apply(const witness_update_operation& o) {
        const auto& owner = _db.get_account(o.owner); // verify owner exists

        const auto &idx = _db.get_index<witness_index>().indices().get<by_name>();
        auto itr = idx.find(o.owner);
        if (itr != idx.end()) {
            _db.modify(*itr, [&](witness_object& w) {
                from_string(w.url, o.url);
                w.signing_key = o.block_signing_key;
            });
        } else {
            if(_db.has_hardfork(CHAIN_HARDFORK_9)){
                const auto& median_props = _db.get_witness_schedule_object().median_props;
                const dynamic_global_property_object &dgp = _db.get_dynamic_global_properties();

                FC_ASSERT(owner.balance >=
                          median_props.witness_declaration_fee, "Account does not have sufficient funds to declare himself as witness: required ${a}.",("a",median_props.witness_declaration_fee));

                _db.adjust_balance(owner, -median_props.witness_declaration_fee);
                _db.modify(dgp, [&](dynamic_global_property_object &dgp) {
                    dgp.committee_fund += median_props.witness_declaration_fee;
                });
            }
            _db.create<witness_object>([&](witness_object& w) {
                w.owner = o.owner;
                from_string(w.url, o.url);
                w.signing_key = o.block_signing_key;
                w.created = _db.head_block_time();
            });
        }
    }

    void chain_properties_update_evaluator::do_apply(const chain_properties_update_operation& o) {
        _db.get_account(o.owner); // verify owner exists

        const auto &idx = _db.get_index<witness_index>().indices().get<by_name>();
        auto itr = idx.find(o.owner);
        if (itr != idx.end()) {
            _db.modify(*itr, [&](witness_object& w) {
                w.props = o.props;
            });
        } else {
            _db.create<witness_object>([&](witness_object& w) {
                w.owner = o.owner;
                w.created = _db.head_block_time();
                w.props = o.props;
            });
        }
    }

    struct chain_properties_update {
        using result_type = void;

        const database& _db;
        chain_properties& _wprops;

        chain_properties_update(const database& db, chain_properties& wprops)
                : _db(db), _wprops(wprops) {
        }

        result_type operator()(const chain_properties_hf4& p) const {
            FC_ASSERT( _db.has_hardfork(CHAIN_HARDFORK_4), "chain_properties_hf4" );
            _wprops = p;
        }

        result_type operator()(const chain_properties_hf6& p) const {
            FC_ASSERT( _db.has_hardfork(CHAIN_HARDFORK_6), "chain_properties_hf6" );
            _wprops = p;
        }

        result_type operator()(const chain_properties_hf9& p) const {
            FC_ASSERT( _db.has_hardfork(CHAIN_HARDFORK_9), "chain_properties_hf9" );
            _wprops = p;
        }

        template<typename Props>
        result_type operator()(Props&& p) const {
            _wprops = p;
        }
    };

    void versioned_chain_properties_update_evaluator::do_apply(const versioned_chain_properties_update_operation& o) {
        FC_ASSERT( _db.has_hardfork(CHAIN_HARDFORK_4), "versioned_chain_properties_update_evaluator not enabled until HF 4" );
        _db.get_account(o.owner); // verify owner exists

        const auto &idx = _db.get_index<witness_index>().indices().get<by_name>();
        auto itr = idx.find(o.owner);
        if (itr != idx.end()) {
            _db.modify(*itr, [&](witness_object& w) {
                o.props.visit(chain_properties_update(_db, w.props));
            });
        } else {
            _db.create<witness_object>([&](witness_object& w) {
                w.owner = o.owner;
                w.created = _db.head_block_time();
                o.props.visit(chain_properties_update(_db, w.props));
            });
        }
    }

} } // graphene::chain