#include <graphene/chain/chain_evaluator.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/chain_objects.hpp>
#include <graphene/chain/invite_objects.hpp>

#include <graphene/utilities/key_conversion.hpp>

namespace graphene { namespace chain {

    void create_invite_evaluator::do_apply(const create_invite_operation& o) {
        const auto& creator = _db.get_account(o.creator);
        const auto& median_props = _db.get_witness_schedule_object().median_props;

        FC_ASSERT(creator.balance >= o.balance, "Insufficient balance to create invite.",
                        ("creator.balance", creator.balance)("invite.balance", o.balance));
        FC_ASSERT(o.invite_key != public_key_type(), "Invite key cannot be blank.");
        FC_ASSERT(o.balance >= median_props.account_creation_fee, "Invite balance must be more or equal consensus account_creation_fee.");

        const auto &idx = _db.get_index<invite_index>().indices().get<by_invite_key>();
        auto itr = idx.find(o.invite_key);
        FC_ASSERT(itr == idx.end(), "Invite with same key was found. Use another invite key.");

        _db.adjust_balance(creator, -o.balance);
        _db.create<invite_object>([&](invite_object& c) {
            c.creator = o.creator;
            c.invite_key = o.invite_key;
            c.balance = o.balance;
            c.create_time = _db.head_block_time();
            c.status = 0;
        });
    }

    void claim_invite_balance_evaluator::do_apply(const claim_invite_balance_operation& o) {
        const auto& receiver = _db.get_account(o.receiver);
        FC_ASSERT(o.invite_secret.size(), "Invite secret cannot be blank.");

        fc::optional<fc::ecc::private_key> invite_secret = graphene::utilities::wif_to_key(o.invite_secret);
        FC_ASSERT(invite_secret, "Invite secret must be WIF.");

        public_key_type check_invite_key = invite_secret->get_public_key();

        const auto &idx = _db.get_index<invite_index>().indices().get<by_invite_key>();
        auto itr = idx.find(check_invite_key);
        FC_ASSERT(itr != idx.end(), "Invite was not found.");

        _db.adjust_balance(receiver, itr->balance);
        if(itr->status == 0){
            _db.modify(*itr, [&](invite_object& c) {
                c.status = 1;
                c.invite_secret.append(o.invite_secret.c_str());
                c.receiver = o.receiver;
                c.claimed_balance=c.balance;
                c.balance=asset(0, TOKEN_SYMBOL);
                c.claim_time = _db.head_block_time();
            });
        }
        else{
            FC_ASSERT(false, "Invite already claimed.");
        }
    }

    void invite_registration_evaluator::do_apply(const invite_registration_operation& o) {
        FC_ASSERT(o.invite_secret.size(), "Invite secret cannot be blank.");

        fc::optional<fc::ecc::private_key> invite_secret = graphene::utilities::wif_to_key(o.invite_secret);
        FC_ASSERT(invite_secret, "Invite secret must be WIF.");

        public_key_type check_invite_key = invite_secret->get_public_key();

        const auto &idx = _db.get_index<invite_index>().indices().get<by_invite_key>();
        auto itr = idx.find(check_invite_key);
        FC_ASSERT(itr != idx.end(), "Invite was not found.");

        if(itr->status == 0){
        	public_key_type key_from_operation(o.new_account_key);

        	_db.create<account_object>([&](account_object &acc) {
        	    acc.name = o.new_account_name;
        	    acc.memo_key = key_from_operation;
        	    acc.created = _db.head_block_time();
        	    acc.recovery_account = itr->creator;
        	    acc.referrer = itr->creator;
        	});
        	_db.create<account_authority_object>([&](account_authority_object &auth) {
        	    auth.account = o.new_account_name;
        	    auth.owner.add_authority(key_from_operation, 1);
        	    auth.owner.weight_threshold = 1;
        	    auth.active = auth.owner;
        	    auth.posting = auth.active;
        	});
        	_db.create<account_metadata_object>([&](account_metadata_object& m) {
        	    m.account = o.new_account_name;
        	});
        	const auto &new_account = _db.get_account(o.new_account_name);
        	_db.create_vesting(new_account, itr->balance);
            _db.modify(*itr, [&](invite_object& c) {
                c.status = 2;
                c.invite_secret.append(o.invite_secret.c_str());
                c.receiver = o.new_account_name;
                c.claimed_balance=c.balance;
                c.balance=asset(0, TOKEN_SYMBOL);
                c.claim_time = _db.head_block_time();
            });
        }
        else{
            FC_ASSERT(false, "Invite already claimed.");
        }
    }
} } // graphene::chain