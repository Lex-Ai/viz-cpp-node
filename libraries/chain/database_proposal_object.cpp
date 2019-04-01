#include <graphene/chain/database.hpp>
#include <graphene/chain/proposal_object.hpp>
#include <graphene/chain/account_object.hpp>

namespace graphene { namespace chain {

    const proposal_object& database::get_proposal(
        const account_name_type& author,
        const std::string& title
    ) const { try {
        return get<proposal_object, by_account>(std::make_tuple(author, title));
    } FC_CAPTURE_AND_RETHROW((author)(title)) }

    const proposal_object *database::find_proposal(
        const account_name_type& author,
        const std::string& title
    ) const {
        return find<proposal_object, by_account>(std::make_tuple(author, title));
    }

    void database::push_proposal(const proposal_object& proposal) { try {
        auto ops = proposal.operations();
        auto session = start_undo_session();
        for (auto& op : ops) {
            apply_operation(op, true);
        }
        // the parent session has been created in _push_block()/_push_transaction()
        session.squash();
        remove(proposal);
    } FC_CAPTURE_AND_RETHROW((proposal.author)(proposal.title)) }

    void database::remove(const proposal_object& p) {
        flat_set<account_name_type> required_total;
        required_total.insert(p.required_active_approvals.begin(), p.required_active_approvals.end());
        required_total.insert(p.required_master_approvals.begin(), p.required_master_approvals.end());
        required_total.insert(p.required_regular_approvals.begin(), p.required_regular_approvals.end());

        auto& idx = get_index<required_approval_index>().indices().get<by_account>();
        for (const auto& account: required_total) {
            auto itr = idx.find(std::make_tuple(account, p.id));
            if (idx.end() != itr) {
                remove(*itr);
            }
        }

        chainbase::database::remove(p);
    }

    void database::clear_expired_proposals() {
        const auto& proposal_expiration_index = get_index<proposal_index>().indices().get<by_expiration>();
        const auto now = head_block_time();

        while (!proposal_expiration_index.empty() && proposal_expiration_index.begin()->expiration_time <= now) {
            const proposal_object& proposal = *proposal_expiration_index.begin();

            try {
                if (proposal.is_authorized_to_execute(*this)) {
                    push_proposal(proposal);
                    continue;
                }
            } catch (const fc::exception& e) {
                elog(
                    "Failed to apply proposed transaction on its expiration. "
                    "Deleting it.\n${author}::${title}\n${error}",
                    ("author", proposal.author)("title", proposal.title)("error", e.to_detail_string()));
            }
            remove(proposal);
        }
    }

} } // graphene::chain