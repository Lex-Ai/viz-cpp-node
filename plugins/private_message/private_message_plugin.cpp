#include <graphene/plugins/private_message/private_message_plugin.hpp>
#include <graphene/plugins/private_message/private_message_evaluators.hpp>
#include <graphene/plugins/chain/plugin.hpp>
#include <appbase/application.hpp>

#include <graphene/chain/index.hpp>
#include <graphene/chain/custom_operation_interpreter.hpp>
#include <graphene/chain/generic_custom_operation_interpreter.hpp>

#include <fc/smart_ref_impl.hpp>


//
template<typename T>
T dejsonify(const std::string &s) {
    return fc::json::from_string(s).as<T>();
}

#define DEFAULT_VALUE_VECTOR(value) default_value({fc::json::to_string(value)}, fc::json::to_string(value))
#define LOAD_VALUE_SET(options, name, container, type) \
if( options.count(name) ) { \
    const std::vector<std::string>& ops = options[name].as<std::vector<std::string>>(); \
    std::transform(ops.begin(), ops.end(), std::inserter(container, container.end()), &dejsonify<type>); \
}
//

namespace graphene {
    namespace plugins {
        namespace private_message {

            class private_message_plugin::private_message_plugin_impl final {
            public:
                private_message_plugin_impl(private_message_plugin &_plugin)
                        : _self(_plugin) ,
                          _db(appbase::app().get_plugin<graphene::plugins::chain::plugin>().db()){
                    _custom_operation_interpreter = std::make_shared
                            < generic_custom_operation_interpreter <private_message::private_message_plugin_operation>> (_db);

                    _custom_operation_interpreter->register_evaluator<private_message_evaluator>(&_self);

                    _db.set_custom_operation_interpreter(_self.name(), _custom_operation_interpreter);
                    return;
                }

                vector <message_api_obj> get_inbox(const std::string& to, time_point newest, uint16_t limit, std::uint64_t offset) const;

                vector <message_api_obj> get_outbox(const std::string& from, time_point newest, uint16_t limit, std::uint64_t offset) const;


                ~private_message_plugin_impl() {};

                private_message_plugin &_self;
                std::shared_ptr <generic_custom_operation_interpreter<private_message_plugin_operation>> _custom_operation_interpreter;
                flat_map <std::string, std::string> _tracked_accounts;

                graphene::chain::database &_db;
            };

            vector <message_api_obj> private_message_plugin::private_message_plugin_impl::get_inbox(
                    const std::string& to, time_point newest, uint16_t limit, std::uint64_t offset) const {
                FC_ASSERT(limit <= 100);

                vector <message_api_obj> result;
                const auto &idx = _db.get_index<message_index>().indices().get<by_to_date>();
                auto itr = idx.lower_bound(std::make_tuple(to, newest));

                if (idx.size() > offset) {
                    while (itr != idx.end() && offset && itr->to == to) {
                        ++itr;
                        --offset;
                    }
                }

                while (itr != idx.end() && limit && itr->to == to) {
                    result.push_back(*itr);
                    ++itr;
                    --limit;
                }

                return result;
            }

            vector <message_api_obj> private_message_plugin::private_message_plugin_impl::get_outbox(
                    const std::string& from, time_point newest, uint16_t limit, std::uint64_t offset) const {
                FC_ASSERT(limit <= 100);

                vector <message_api_obj> result;
                const auto &idx = _db.get_index<message_index>().indices().get<by_from_date>();

                auto itr = idx.lower_bound(std::make_tuple(from, newest));

                if (idx.size() > offset) {
                    while (itr != idx.end() && offset && itr->from == from) {
                        ++itr;
                        --offset;
                    }
                }

                while (itr != idx.end() && limit && itr->from == from) {
                    result.push_back(*itr);
                    ++itr;
                    --limit;
                }
                return result;
            }

            void private_message_evaluator::do_apply(const private_message_operation &pm) {
                database &d = db();

                const flat_map <std::string, std::string> &tracked_accounts = _plugin->tracked_accounts();

                auto to_itr = tracked_accounts.lower_bound(pm.to);
                auto from_itr = tracked_accounts.lower_bound(pm.from);

                FC_ASSERT(pm.from != pm.to);
                FC_ASSERT(pm.from_memo_key != pm.to_memo_key);
                FC_ASSERT(pm.sent_time != 0);
                FC_ASSERT(pm.encrypted_message.size() >= 32);

                if (!tracked_accounts.size() ||
                    (to_itr != tracked_accounts.end() && pm.to >= to_itr->first &&
                     pm.to <= to_itr->second) ||
                    (from_itr != tracked_accounts.end() &&
                     pm.from >= from_itr->first && pm.from <= from_itr->second)) {
                    d.create<message_object>([&](message_object &pmo) {
                        pmo.from = pm.from;
                        pmo.to = pm.to;
                        pmo.from_memo_key = pm.from_memo_key;
                        pmo.to_memo_key = pm.to_memo_key;
                        pmo.checksum = pm.checksum;
                        pmo.sent_time = pm.sent_time;
                        pmo.receive_time = d.head_block_time();
                        pmo.encrypted_message.resize(pm.encrypted_message.size());
                        std::copy(pm.encrypted_message.begin(), pm.encrypted_message.end(),
                                  pmo.encrypted_message.begin());
                    });
                }
            }

            private_message_plugin::private_message_plugin(){
            }

            private_message_plugin::~private_message_plugin() {
            }

            void private_message_plugin::set_program_options(
                    boost::program_options::options_description &cli,
                    boost::program_options::options_description &cfg) {
                cli.add_options()
                        ("pm-account-range",
                         boost::program_options::value < std::vector < std::string >> ()->composing()->multitoken(),
                         "Defines a range of accounts to private messages to/from as a json pair [\"from\",\"to\"] [from,to)");
                cfg.add(cli);
            }

            void private_message_plugin::plugin_initialize(const boost::program_options::variables_map &options) {
                ilog("Intializing private message plugin");
                my.reset(new private_message_plugin::private_message_plugin_impl(*this));

                add_plugin_index<message_index>(my->_db);

                typedef pair <string, string> pairstring;
                LOAD_VALUE_SET(options, "pm-accounts", my->_tracked_accounts, pairstring);
                JSON_RPC_REGISTER_API(name())
            }

            void private_message_plugin::plugin_startup() {
                ilog("Starting up private message plugin");
            }

            void private_message_plugin::plugin_shutdown() {
                ilog("Shuting down private message plugin");
            }

            flat_map <string, string> private_message_plugin::tracked_accounts() const {
                return my->_tracked_accounts;
            }

            // Api Defines

            DEFINE_API(private_message_plugin, get_inbox) {
                auto to = args.args->at(0).as<std::string>();
                auto newest = args.args->at(1).as<time_point>();
                auto limit = args.args->at(2).as<uint16_t>();
                auto offset = args.args->at(3).as<std::uint64_t>();
                auto &db = my->_db;
                return db.with_weak_read_lock([&]() {
                    return my->get_inbox(to, newest, limit, offset);
                });
            }

            DEFINE_API(private_message_plugin, get_outbox) {
                auto from = args.args->at(0).as<std::string>();
                auto newest = args.args->at(1).as<time_point>();
                auto limit = args.args->at(2).as<uint16_t>();
                auto offset = args.args->at(3).as<std::uint64_t>();
                auto &db = my->_db;
                return db.with_weak_read_lock([&]() {
                    return my->get_outbox(from, newest, limit, offset);
                });
            }
        }
    }
} // graphene::plugins::private_message
