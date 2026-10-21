#include "civ5/runtime/bnw_grand_conquest_category.hpp"
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
  if constexpr(requires{v.rows;}){if constexpr(requires{v.rows.size();})nodes(v.rows);else n(v.rows);}
  if constexpr(requires{v.hashes;}){if constexpr(requires{v.hashes.front().name;})nodes(v.hashes);else values(v.hashes);}
 }
};
void rng(Digest &h,const LiveSessionState &s){h.n(static_cast<unsigned>(s.world.progression.layout));for(const auto &v:s.world.progression.random){h.n(v.seed);h.n(v.calls);h.n(v.resets);}for(const auto *v:{&s.random.map,&s.random.gameplay}){h.n(static_cast<unsigned>(v->algorithm()));h.n(v->state().seed);h.n(v->state().calls);h.n(v->state().resets);}}
// Raw/owned inventory seal, never a repeated GetBase/category calculator.
// Optional/missing inventories are hashed as they stand; a disabled source
// branch does not acquire requirements for a team/trait getter it never read.
std::string revision(const BnwTeamTechLuaEpoch &e,std::int64_t p){const auto &s=*e.world;Digest h;h.n(p);h.t(bnw_grand_native_component_revision(s,p));h.t(bnw_grand_military_inputs_revision(s));rng(h,s);rng(h,*e.source_world);
 h.n(s.world.all_players.has_value());if(s.world.all_players){const auto &v=s.world.all_players->players.at(p);h.node(v.knowledge);h.n(v.knowledge.profile_extensions.values.size());h.node(v.diplomacy);}
 h.n(s.world.pregame.has_value());if(s.world.pregame){const auto &a=s.world.pregame->archive;for(const auto key:{"num_victory_infos"}){const auto v=a.integers.find(key);h.t(key);h.n(v!=a.integers.end());if(v!=a.integers.end())h.n(v->second);}for(const auto key:{"victories","minor_nation_civs"}){const auto v=a.flag_arrays.find(key);h.t(key);h.n(v!=a.flag_arrays.end());if(v!=a.flag_arrays.end())h.values(v->second);}}
 h.n(s.world.map_projection.has_value());if(s.world.map_projection){const auto &map=*s.world.map_projection;h.n(map.grid.width);h.n(map.grid.height);h.n(map.cells.size());for(const auto &c:map.cells){h.n(c.index);h.n(c.x);h.n(c.y);h.opt(c.owner);}}
 h.n(s.world.map_plots.has_value());if(s.world.map_plots){const auto &map=*s.world.map_plots;h.n(static_cast<unsigned>(map.layout));h.n(map.plots.size());for(const auto &c:map.plots){h.n(c.index);h.n(c.x);h.n(c.y);h.nodes(c.fields);}}
 return identify_bytes(h.bytes).sha256;
}
void binding(const BnwTeamTechLuaEpoch &e,const SessionCheckpointIdentity &id,const SavedContentProfile &profile,const BnwGrandWorldAverageRules &r,const BnwHappinessSuffixRules &n,const content::RulesDatabase &db){need(e.world&&e.host&&e.source_world&&e.source_world!=e.world.get()&&e.getter_world==e.world.get()&&e.identity.payload_sha256==id.payload_sha256&&e.identity.rules_sha256==id.rules_sha256&&e.selected_dlc==profile.enabled_dlc&&e.selected_mods==profile.enabled_mods&&id.payload_sha256.size()==64&&id.rules_sha256==r.identity()&&r.identity()==n.identity()&&identify_bytes(db.gameplay_fingerprint_data()).sha256==id.rules_sha256&&profile.can_activate&&profile.all_requirements_located&&profile.enabled_mods.empty(),"BNW Conquest category SAME world/VM/source/rules binding invalid");need(e.all_selected_contexts_loaded&&e.all_listeners_registered&&e.all_external_writes_escrowed&&e.host->loading_complete()&&e.host->source_errors().empty(),"BNW Conquest category retained VM/loading/escrow invalid");}
}
BnwGrandConquestPriorityJournal stage_bnw_grand_conquest_category(const BnwGameActivationHappinessPending &enclosing,const BnwTeamTechLuaEpoch &e,const BnwGrandWorldAverageRules &r,const BnwHappinessSuffixRules &n,const content::RulesDatabase &db){
 need(enclosing.activation_&&enclosing.happiness_&&enclosing.builder_complete_&&enclosing.diplomacy_begin_complete_&&enclosing.completed_prefix_.size()==14&&enclosing.completed_prefix_.back()==BnwGameActivationStep::diplomacy_requests_begin&&enclosing.happiness_->native_update_complete()&&&enclosing.state()==e.world.get(),"BNW Conquest category requires genuine SAME enclosing native prefix");need(enclosing.happiness_->suffix_&&enclosing.happiness_->prefix_&&enclosing.happiness_->suffix_->continuation.religion.query.epoch.get()==&e&&e.source_world==&enclosing.happiness_->prefix_->state(),"BNW Conquest category requires exact retained suffix epoch/prefix source");need(!enclosing.happiness_->grand_conquest_category(),"BNW Conquest source category already consumed; no replay");const auto *roll=enclosing.happiness_->grand_third_roll();const auto &owner=enclosing.activation_->owner;const auto p=enclosing.ticket_.player;need(roll&&roll->player()==p&&roll->identity().payload_sha256==owner.identity.payload_sha256&&roll->identity().rules_sha256==owner.identity.rules_sha256,"BNW Conquest category requires actual private third roll predecessor");
 (void)checked_bnw_grand_third_roll(*roll,e,r,n,db);binding(e,owner.identity,roll->profile(),r,n,db);
 BnwGrandConquestPriorityJournal j;j.identity_=owner.identity;j.profile_=roll->profile();j.player_=p;j.world_=e.world.get();j.source_=e.source_world;j.host_=e.host.get();const auto base_rules=BnwGrandPriorityRules::load(db);
 // The original cached military loop is invoked again, precisely at source439
 // after GetBase and the enabled category prefix. Never reuse DoGuess's journal.
 const std::function<const BnwGrandMilitaryAverageJournal &()> current_average=[&]()->const BnwGrandMilitaryAverageJournal&{need(!j.progress_.military_average,"BNW Conquest source military average unexpectedly repeated");j.progress_.military_average.emplace(query_bnw_grand_current_military_average(*e.world,n,db,owner.identity,p));return *j.progress_.military_average;};
 auto q=quote_bnw_grand_conquest_priority_at_source(*e.world,base_rules,n,db,owner.identity,p,current_average);need(q.base_quoted&&q.base&&q.category_quoted&&q.category&&q.before_random&&q.strategy==3&&q.strategy_type=="AIGRANDSTRATEGY_CONQUEST",q.blocker.c_str());need(q.military_ratio_query==j.progress_.military_average.has_value(),"BNW Conquest fresh military query/source gate differs");j.progress_.before_random=*q.before_random;j.progress_.priority=std::move(q);j.revision_=revision(e,p);return j;
}
const BnwGrandConquestCategoryProgress &checked_bnw_grand_conquest_category(const BnwGrandConquestPriorityJournal &j,const BnwTeamTechLuaEpoch &e,const BnwGrandWorldAverageRules &r,const BnwHappinessSuffixRules &n,const content::RulesDatabase &db){binding(e,j.identity_,j.profile_,r,n,db);need(e.world.get()==j.world_&&e.source_world==j.source_&&e.host.get()==j.host_,"BNW Conquest category completed SAME retained allocation changed");need(revision(e,j.player_)==j.revision_,"BNW Conquest category completed source inputs stale");return j.progress_;}
}
