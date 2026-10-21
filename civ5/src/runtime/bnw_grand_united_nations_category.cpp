#include "civ5/runtime/bnw_grand_united_nations_category.hpp"
#include "civ5/runtime/bnw_game_activation_execution.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <climits>
#include <type_traits>
namespace civ5::runtime {
namespace {
void need(bool v,const char *why){if(!v)throw std::runtime_error(why);}
struct Digest {
 std::vector<std::uint8_t> bytes;
 void n(std::uint64_t v){for(unsigned k=0;k<8;++k)bytes.push_back(static_cast<std::uint8_t>(v>>(8*k)));}
 void t(std::string_view v){n(v.size());bytes.insert(bytes.end(),v.begin(),v.end());}
 template<class T>void opt(const std::optional<T> &v){n(v.has_value());if(v)n(*v);}
 template<class T>void values(const T &v){n(v.size());for(auto a:v)n(a);}
 template<class T>void nodes(const T &v){n(v.size());for(const auto &a:v)node(a);}
 template<class T>void node(const T &v){
  if constexpr(requires{v.name;})t(v.name?v.name:"");
  if constexpr(requires{v.index;})n(v.index);
  if constexpr(requires{v.layout;})n(static_cast<unsigned>(v.layout));
  if constexpr(requires{v.start_offset;})n(v.start_offset);
  if constexpr(requires{v.end_offset;})n(v.end_offset);
  if constexpr(requires{v.version;})opt(v.version);
  if constexpr(requires{v.encoding;})n(static_cast<unsigned>(v.encoding));
  if constexpr(requires{v.columns;})n(v.columns);
  if constexpr(requires{v.value;})n(v.value);
  if constexpr(requires{v.type_hash;})n(v.type_hash);
  if constexpr(requires{v.wire_count;}){if constexpr(requires{v.wire_count.has_value();})opt(v.wire_count);else n(v.wire_count);}
  if constexpr(requires{v.key;})n(v.key);
  if constexpr(requires{v.values;})values(v.values);
  if constexpr(requires{v.bytes;})values(v.bytes);
  if constexpr(requires{v.row_keys;})values(v.row_keys);
  if constexpr(requires{v.row_counts;})values(v.row_counts);
  if constexpr(requires{v.fields;})nodes(v.fields);
  if constexpr(requires{v.arrays;})nodes(v.arrays);
  if constexpr(requires{v.blocks;})nodes(v.blocks);
  if constexpr(requires{v.rows;})nodes(v.rows);
  if constexpr(requires{v.hashes;}){if constexpr(requires{v.hashes.front().name;})nodes(v.hashes);else values(v.hashes);}
 }
};
void rng(Digest &h,const LiveSessionState &s){h.n(static_cast<unsigned>(s.world.progression.layout));for(const auto &v:s.world.progression.random){h.n(v.seed);h.n(v.calls);h.n(v.resets);}for(const auto *v:{&s.random.map,&s.random.gameplay}){h.n(static_cast<unsigned>(v->algorithm()));h.n(v->state().seed);h.n(v->state().calls);h.n(v->state().resets);}}
// Raw/owned inventory seal, never a repeated GetBase/category calculator.
// Optional/missing inventories are hashed as they stand; a disabled source
// branch does not acquire requirements for a team/trait getter it never read.
std::string revision(const BnwTeamTechLuaEpoch &e,std::int64_t p){const auto &s=*e.world;Digest h;h.n(p);h.n(static_cast<unsigned>(s.setup.profile));h.t(bnw_grand_native_component_revision(s,p));rng(h,s);rng(h,*e.source_world);
 h.n(s.world.all_players.has_value());if(s.world.all_players){h.n(static_cast<unsigned>(s.world.all_players->layout));h.n(s.world.all_players->players.size());const auto &v=s.world.all_players->players.at(p);h.n(v.index);h.n(v.saved_player_id);h.node(v.core);h.node(v.knowledge);h.n(v.knowledge.profile_extensions.values.size());h.node(v.traits);h.n(v.traits.dimensions.yields);h.n(v.traits.dimensions.terrains);}
 h.n(s.world.pregame.has_value());if(s.world.pregame){h.n(static_cast<unsigned>(s.world.pregame->layout));const auto &a=s.world.pregame->archive;for(const auto key:{"num_victory_infos"}){const auto it=a.integers.find(key);h.n(it!=a.integers.end());if(it!=a.integers.end())h.n(it->second);}for(const auto key:{"victories"}){const auto it=a.flag_arrays.find(key);h.n(it!=a.flag_arrays.end());if(it!=a.flag_arrays.end())h.values(it->second);}for(const auto key:{"team_types"}){const auto it=a.integer_arrays.find(key);h.n(it!=a.integer_arrays.end());if(it!=a.integer_arrays.end())h.values(it->second);}}
 h.n(s.world.teams.has_value());if(s.world.teams){h.n(static_cast<unsigned>(s.world.teams->layout));h.n(s.world.teams->dimensions.teams);h.n(s.world.teams->teams.size());for(const auto &v:s.world.teams->teams){h.n(v.index);h.n(v.start_offset);h.n(v.end_offset);h.nodes(v.fields);}}
 const auto own=s.setup.players.find(p);h.n(own!=s.setup.players.end());if(own!=s.setup.players.end()){h.n(own->second.alive);h.n(own->second.team);}h.n(static_cast<unsigned>(s.maintenance.profile));h.n(s.maintenance.elapsed_turns);h.n(s.maintenance.team_eras.size());for(const auto &[t,era]:s.maintenance.team_eras){h.n(t);h.n(era);}
 h.n(static_cast<unsigned>(s.world.game_state.layout));h.n(s.world.game_state.prefix.index());if(const auto *v=std::get_if<saves::Expansion2PrefixObservation>(&s.world.game_state.prefix)){h.n(v->version);h.n(v->end_offset);h.values(v->scalars);}
 h.node(s.world.leagues);h.n(s.world.leagues.leagues.size());h.n(s.cities.leagues.size());return identify_bytes(h.bytes).sha256;
}
void binding(const BnwTeamTechLuaEpoch &e,const SessionCheckpointIdentity &id,const SavedContentProfile &profile,const BnwGrandWorldAverageRules &r,const BnwHappinessSuffixRules &n,const content::RulesDatabase &db){need(e.world&&e.host&&e.source_world&&e.source_world!=e.world.get()&&e.getter_world==e.world.get()&&e.identity.payload_sha256==id.payload_sha256&&e.identity.rules_sha256==id.rules_sha256&&e.selected_dlc==profile.enabled_dlc&&e.selected_mods==profile.enabled_mods&&id.payload_sha256.size()==64&&id.rules_sha256==r.identity()&&r.identity()==n.identity()&&identify_bytes(db.gameplay_fingerprint_data()).sha256==id.rules_sha256&&profile.can_activate&&profile.all_requirements_located&&profile.enabled_mods.empty(),"BNW UN category SAME world/VM/source/rules binding invalid");need(e.all_selected_contexts_loaded&&e.all_listeners_registered&&e.all_external_writes_escrowed&&e.host->loading_complete()&&e.host->source_errors().empty(),"BNW UN category retained VM/loading/escrow invalid");}
}
BnwGrandUnitedNationsPriorityJournal stage_bnw_grand_united_nations_category(const BnwGameActivationHappinessPending &enclosing,const BnwTeamTechLuaEpoch &e,const BnwGrandWorldAverageRules &r,const BnwHappinessSuffixRules &n,const content::RulesDatabase &db){
 need(enclosing.activation_&&enclosing.happiness_&&enclosing.builder_complete_&&enclosing.diplomacy_begin_complete_&&enclosing.completed_prefix_.size()==14&&enclosing.completed_prefix_.back()==BnwGameActivationStep::diplomacy_requests_begin&&enclosing.happiness_->native_update_complete()&&&enclosing.state()==e.world.get(),"BNW UN category requires genuine SAME enclosing native prefix");need(enclosing.happiness_->suffix_&&enclosing.happiness_->prefix_&&enclosing.happiness_->suffix_->continuation.religion.query.epoch.get()==&e&&e.source_world==&enclosing.happiness_->prefix_->state(),"BNW UN category requires exact retained suffix epoch/prefix source");const auto *roll=enclosing.happiness_->grand_first_roll();const auto &owner=enclosing.activation_->owner;const auto p=enclosing.ticket_.player;need(roll&&roll->player()==p&&roll->identity().payload_sha256==owner.identity.payload_sha256&&roll->identity().rules_sha256==owner.identity.rules_sha256,"BNW UN category requires actual private first roll predecessor");
 (void)checked_bnw_grand_first_roll(*roll,e,r,n,db);binding(e,owner.identity,roll->profile(),r,n,db);
 const auto base_rules=BnwGrandPriorityRules::load(db);auto base=quote_bnw_grand_base_priority(*e.world,base_rules,n,db,owner.identity,p,1);need(base.base_quoted&&base.base&&base.strategy==1&&base.strategy_type=="AIGRANDSTRATEGY_UNITED_NATIONS"&&!base.category_quoted&&!base.category&&!base.before_random,base.blocker.c_str());auto category=quote_bnw_grand_united_nations_priority(*e.world,n,db,owner.identity,p);need(category.calculated&&category.category&&!category.starting_votes_pending,category.blocker.c_str());const auto sum=static_cast<std::int64_t>(*base.base)+*category.category;need(sum>=INT32_MIN&&sum<=INT32_MAX,"BNW UN category native before-random overflow");
 BnwGrandUnitedNationsPriorityJournal j;j.identity_=owner.identity;j.profile_=roll->profile();j.player_=p;j.world_=e.world.get();j.source_=e.source_world;j.host_=e.host.get();j.progress_.base=std::move(base);j.progress_.category=std::move(category);j.progress_.before_random=static_cast<std::int32_t>(sum);j.revision_=revision(e,p);return j;
}
const BnwGrandUnitedNationsCategoryProgress &checked_bnw_grand_united_nations_category(const BnwGrandUnitedNationsPriorityJournal &j,const BnwTeamTechLuaEpoch &e,const BnwGrandWorldAverageRules &r,const BnwHappinessSuffixRules &n,const content::RulesDatabase &db){binding(e,j.identity_,j.profile_,r,n,db);need(e.world.get()==j.world_&&e.source_world==j.source_&&e.host.get()==j.host_,"BNW UN category completed SAME retained allocation changed");need(revision(e,j.player_)==j.revision_,"BNW UN category completed source inputs stale");return j.progress_;}
}
